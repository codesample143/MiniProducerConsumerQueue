#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "queue.hpp" // Assuming the class is saved here

int main() {
    // 1. Initialize our lock-free queue. 
    // The capacity must be a power of two (e.g., 1024) for our bitwise mask to work.
    const size_t CAPACITY = 1024;
    LockFreeJobQueue queue(CAPACITY);
    std::atomic<bool> running{true};

    const int num_producers = 4;
    const int num_consumers = 4;
    const int jobs_per_producer = 10000;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> jobs_processed{0};

    std::cout << "Starting " << num_producers << " producers and " << num_consumers << " consumers..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    //starting threads
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&queue, &running, &jobs_processed]() {
            while (running || std::optional<std::function<void()>>{}) {
                auto job_opt = queue.try_pop();
                if (job_opt) {
                    (*job_opt)(); 
                    jobs_processed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    if (!running) {
                        break;
                    }
                    std::this_thread::yield(); 
                }
            }
        });
    }
    //producer
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&queue, jobs_per_producer, i]() {
            for (int j = 0; j < jobs_per_producer; ++j) {
                auto job = [i, j]() {
                    // For demo purposes, we keep the work light and do absolutely nothing.
                };
                while (!queue.try_push(job)) {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producers) {
        if (t.joinable()) {
            t.join();
        }
    }
    std::cout << "All producers finished generating jobs." << std::endl;
    while (true) {
        if (jobs_processed.load() == (num_producers * jobs_per_producer)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    running.store(false, std::memory_order_release);

    // Join consumer threads
    for (auto& t : consumers) {
        if (t.joinable()) {
            t.join();
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

    // 6. Verify Results print statements (im not writing this shit out)
    std::cout << "\n--- Run Summary ---" << std::endl;
    std::cout << "Expected Jobs: " << (num_producers * jobs_per_producer) << std::endl;
    std::cout << "Actual Processed: " << jobs_processed.load() << std::endl;
    std::cout << "Execution Time: " << elapsed.count() << " ms" << std::endl;

    return 0;
}