#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <vector>
#include <chrono>

enum SchedulingPolicy {
    ROUND_ROBIN,
    SHORTEST_JOB_FIRST
};

struct Task {
    std::function<void()> function;
    uint32_t estimatedTime; // For SJF scheduling
    uint32_t taskID;
    
    Task(std::function<void()> func, uint32_t est = 1, uint32_t id = 0)
        : function(func), estimatedTime(est), taskID(id) {}
    
    bool operator<(const Task& other) const {
        // For priority queue (min heap) - shortest time first
        return estimatedTime > other.estimatedTime;
    }
};

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<Task> rrQueue; // Round Robin queue
    std::priority_queue<Task> sjfQueue; // SJF priority queue
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    SchedulingPolicy policy;
    
    // Statistics
    uint64_t tasksProcessed;
    uint64_t totalWaitTime;
    std::mutex statsMutex;
    
public:
    ThreadPool(size_t threads, SchedulingPolicy p = ROUND_ROBIN) 
        : stop(false), policy(p), tasksProcessed(0), totalWaitTime(0) {
        
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    Task task([](){}, 0, 0);
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { 
                            return stop || !rrQueue.empty() || !sjfQueue.empty(); 
                        });
                        
                        if (stop && rrQueue.empty() && sjfQueue.empty()) 
                            return;
                        
                        if (policy == ROUND_ROBIN && !rrQueue.empty()) {
                            task = std::move(rrQueue.front());
                            rrQueue.pop();
                        } else if (policy == SHORTEST_JOB_FIRST && !sjfQueue.empty()) {
                            task = sjfQueue.top();
                            sjfQueue.pop();
                        } else {
                            continue;
                        }
                    }
                    
                    auto start = std::chrono::steady_clock::now();
                    task.function();
                    auto end = std::chrono::steady_clock::now();
                    
                    {
                        std::lock_guard<std::mutex> lock(statsMutex);
                        tasksProcessed++;
                        totalWaitTime += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                    }
                }
            });
        }
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    void enqueue(std::function<void()> task, uint32_t estimatedTime = 1, uint32_t taskID = 0) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (policy == ROUND_ROBIN) {
                rrQueue.emplace(task, estimatedTime, taskID);
            } else {
                sjfQueue.emplace(task, estimatedTime, taskID);
            }
        }
        condition.notify_one();
    }
    
    void getStats(uint64_t& processed, uint64_t& avgWaitTime) {
        std::lock_guard<std::mutex> lock(statsMutex);
        processed = tasksProcessed;
        avgWaitTime = (tasksProcessed > 0) ? (totalWaitTime / tasksProcessed) : 0;
    }
};

#endif // THREAD_POOL_H
