#include "thread_models.hpp"
#include <iostream>
#include <chrono>

// Simulated task for threads
void simulatedTask(int id) {
    std::cout << "Thread " << id << " starting work\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 1000));
    std::cout << "Thread " << id << " finished work\n";
}

void demonstrateModel(ThreadingModel& model, const std::string& modelName) {
    std::cout << "\nDemonstrating " << modelName << " Model\n";
    std::cout << "================================\n";

    model.start();

    // Create 5 user threads with simulated tasks
    for (int i = 0; i < 5; ++i) {
        model.addUserThread([i]() { simulatedTask(i); });
    }

    // Let the simulation run for a while
    std::this_thread::sleep_for(std::chrono::seconds(10));
    model.stop();
}

int main() {
    std::cout << "Threading Models Simulator\n";
    std::cout << "=========================\n";

    // Demonstrate Many-to-One model
    {
        ManyToOne model;
        demonstrateModel(model, "Many-to-One");
    }

    // Demonstrate One-to-Many model
    {
        OneToMany model(3);  // 3 kernel threads
        demonstrateModel(model, "One-to-Many");
    }

    // Demonstrate Many-to-Many model
    {
        ManyToMany model(3);  // 3 kernel threads
        demonstrateModel(model, "Many-to-Many");
    }

    return 0;
}
