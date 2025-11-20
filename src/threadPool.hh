#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <functional>
#include <queue>

using namespace std;

// Define the thread pool class
class ThreadPool {
public:
    ThreadPool(size_t thread_count) : stop(false) {
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([this] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }
                        task = move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<typename F, typename... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            unique_lock<mutex> lock(queue_mutex);
            tasks.emplace([=] { f(args...); });
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (thread& thread : threads) {
            thread.join();
        }
    }

private:
    vector<thread> threads;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable condition;
    bool stop;
};
