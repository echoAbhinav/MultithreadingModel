#include "thread_models.hpp"
#include <random>
#include <sstream>

UserThread::UserThread(int id, std::function<void()> task)
    : id_(id), task_(task), state_(ThreadState::READY) {}

void UserThread::setState(ThreadState state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    state_ = state;
}

void UserThread::execute() {
    setState(ThreadState::RUNNING);
    task_();
    setState(ThreadState::TERMINATED);
}

KernelThread::KernelThread(int id) : id_(id) {}

void KernelThread::start() {
    running_ = true;
    available_ = true;
}

void KernelThread::stop() {
    running_ = false;
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

ThreadingModel::ThreadingModel(const std::string& name) : name_(name) {}

void ThreadingModel::stop() {
    running_ = false;
    cv_.notify_all();
}

void ThreadingModel::addUserThread(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(model_mutex_);
    auto thread = std::make_unique<UserThread>(user_threads_.size(), task);
    ready_queue_.push(thread.get());
    user_threads_.push_back(std::move(thread));
    cv_.notify_one();
}

void ThreadingModel::displayStatus() const {
    std::cout << "\n=== " << name_ << " Model Status ===\n";
    std::cout << std::setw(10) << "Thread ID" << std::setw(15) << "State" << "\n";
    std::cout << std::string(30, '-') << "\n";

    for (const auto& thread : user_threads_) {
        std::string state;
        switch (thread->getState()) {
            case ThreadState::READY: state = "READY"; break;
            case ThreadState::RUNNING: state = "RUNNING"; break;
            case ThreadState::BLOCKED: state = "BLOCKED"; break;
            case ThreadState::TERMINATED: state = "TERMINATED"; break;
        }
        std::cout << std::setw(10) << thread->getId() << std::setw(15) << state << "\n";
    }
    std::cout << std::string(30, '-') << "\n";
}

// Many-to-One Implementation
ManyToOne::ManyToOne() : ThreadingModel("Many-to-One") {
    kernel_thread_ = std::make_unique<KernelThread>(0);
}

void ManyToOne::start() {
    kernel_thread_->thread_ = std::make_unique<std::thread>(&ManyToOne::kernelThreadFunc, this);
}

void ManyToOne::kernelThreadFunc() {
    while (running_) {
        std::unique_lock<std::mutex> lock(model_mutex_);
        cv_.wait(lock, [this] { return !ready_queue_.empty() || !running_; });

        if (!running_) break;

        if (!ready_queue_.empty()) {
            auto* thread = ready_queue_.front();
            ready_queue_.pop();
            lock.unlock();

            thread->execute();
            displayStatus();
        }
    }
    kernel_thread_->stop();
}

// One-to-Many Implementation
OneToMany::OneToMany(int num_kernel_threads) : ThreadingModel("One-to-Many") {
    for (int i = 0; i < num_kernel_threads; ++i) {
        kernel_threads_.push_back(std::make_unique<KernelThread>(i));
    }
}

void OneToMany::start() {
    for (auto& kt : kernel_threads_) {
        kt->start();
        kt->thread_ = std::make_unique<std::thread>(&OneToMany::kernelThreadFunc, this, kt.get());
    }
}

void OneToMany::kernelThreadFunc(KernelThread* kernel_thread) {
    while (running_) {
        std::unique_lock<std::mutex> lock(model_mutex_);
        cv_.wait(lock, [this] { return !ready_queue_.empty() || !running_; });

        if (!running_) break;

        if (!ready_queue_.empty() && kernel_thread->isAvailable()) {
            kernel_thread->available_ = false;
            auto* thread = ready_queue_.front();
            ready_queue_.pop();
            lock.unlock();

            thread->execute();
            kernel_thread->available_ = true;
            displayStatus();
        }
    }
}

// Many-to-Many Implementation
ManyToMany::ManyToMany(int num_kernel_threads) : ThreadingModel("Many-to-Many") {
    for (int i = 0; i < num_kernel_threads; ++i) {
        kernel_threads_.push_back(std::make_unique<KernelThread>(i));
    }
}

void ManyToMany::start() {
    for (auto& kt : kernel_threads_) {
        kt->start();
        kt->thread_ = std::make_unique<std::thread>(&ManyToMany::kernelThreadFunc, this, kt.get());
    }
    scheduler_thread_ = std::make_unique<std::thread>(&ManyToMany::schedulerFunc, this);
}

void ManyToMany::schedulerFunc() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::lock_guard<std::mutex> lock(model_mutex_);

        for (auto& thread : user_threads_) {
            if (thread->getState() == ThreadState::RUNNING) {
                if (dis(gen) < 0.2) {  // 20% chance to block a running thread
                    thread->setState(ThreadState::BLOCKED);
                    std::thread([this, thread = thread.get()]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 1500));
                        thread->setState(ThreadState::READY);
                        ready_queue_.push(thread);
                        cv_.notify_one();
                    }).detach();
                }
            }
        }
        displayStatus();
    }
}

void ManyToMany::kernelThreadFunc(KernelThread* kernel_thread) {
    while (running_) {
        std::unique_lock<std::mutex> lock(model_mutex_);
        cv_.wait(lock, [this] { return !ready_queue_.empty() || !running_; });

        if (!running_) break;

        if (!ready_queue_.empty() && kernel_thread->isAvailable()) {
            kernel_thread->available_ = false;
            auto* thread = ready_queue_.front();
            ready_queue_.pop();
            lock.unlock();

            thread->execute();
            kernel_thread->available_ = true;
            displayStatus();
        }
    }
}
