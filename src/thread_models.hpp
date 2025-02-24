#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <iostream>
#include <iomanip>

enum class ThreadState {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// Forward declarations
class UserThread;
class KernelThread;
class ThreadingModel;

class UserThread {
public:
    UserThread(int id, std::function<void()> task);
    int getId() const { return id_; }
    ThreadState getState() const { return state_; }
    void setState(ThreadState state);
    void execute();

private:
    int id_;
    std::function<void()> task_;
    ThreadState state_;
    std::mutex state_mutex_;
    friend class ThreadingModel;
};

class KernelThread {
public:
    KernelThread(int id);
    void start();
    void stop();
    int getId() const { return id_; }
    bool isAvailable() const { return available_; }

protected:
    int id_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_{true};
    std::atomic<bool> available_{true};
    friend class ManyToOne;
    friend class OneToMany;
    friend class ManyToMany;
};

class ThreadingModel {
public:
    ThreadingModel(const std::string& name);
    virtual ~ThreadingModel() = default;

    virtual void start() = 0;
    virtual void stop();
    void addUserThread(std::function<void()> task);
    void displayStatus() const;

protected:
    std::string name_;
    std::vector<std::unique_ptr<UserThread>> user_threads_;
    std::vector<std::unique_ptr<KernelThread>> kernel_threads_;
    std::queue<UserThread*> ready_queue_;
    std::mutex model_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
};

class ManyToOne : public ThreadingModel {
public:
    ManyToOne();
    void start() override;

private:
    void kernelThreadFunc();
    std::unique_ptr<KernelThread> kernel_thread_;
};

class OneToMany : public ThreadingModel {
public:
    OneToMany(int num_kernel_threads = 3);
    void start() override;

private:
    void kernelThreadFunc(KernelThread* kernel_thread);
};

class ManyToMany : public ThreadingModel {
public:
    ManyToMany(int num_kernel_threads = 3);
    void start() override;

private:
    void schedulerFunc();
    void kernelThreadFunc(KernelThread* kernel_thread);
    std::unique_ptr<std::thread> scheduler_thread_;
};
