/*
* Author: Max Brinegar
* Source: https://github.com/MaxBrinegar/thread_pool
* License: MIT
*/

#ifndef THREAD_EXT_THREAD_POOL_H_
#define THREAD_EXT_THREAD_POOL_H_

#include <thread>
#include <vector>
#include <future>
#include <chrono>
#include <shared_mutex>
#include <queue>
#include <optional>
#include <type_traits>
#include <condition_variable>

namespace thread_ext {

class ThreadPool {
    public:
        ThreadPool(size_t thread_count)
            : active_(true) {
            threads_.reserve(thread_count);

            for (size_t i = 0; i < thread_count; ++i) {
                threads_.push_back(std::thread { [this]() {
                    std::function<void()> task;
                    while (true) {
                        {
                            auto lock = std::unique_lock(mutex_);
                            cv_.wait(lock, [this]() { return !queue_.empty() || !active_; });
                            if (!active_) {
                                return;
                            }
                            task = std::move(queue_.front());
                            queue_.pop();
                        }

                        task();
                    }
                } });
            }
        }

        ~ThreadPool() {
            shutdown();
        }

        template<class T, class U = typename std::enable_if<!std::is_same<T, void>::value>::type>
        std::optional<std::future<T>> submit(const std::function<T()>& task) {
            auto p = std::make_shared<std::promise<T>>();
            if (!p) {
                return std::nullopt;
            }

            return submit_inner<T>(p, [p, task]() {
                T res = task();
                p->set_value(std::move(res));
            });
        }

        template<class T, class U = typename std::enable_if<std::is_same<T, void>::value>::type>
        std::optional<std::future<void>> submit(const std::function<void()>& task) {
            auto p = std::make_shared<std::promise<void>>();
            if (!p) {
                return std::nullopt;
            }
            
            return submit_inner<void>(p, [p, task]() {
                task();
                p->set_value();
            });
        }

        template<class T, class U = typename std::enable_if<!std::is_same<T, void>::value>::type>
        std::optional<std::future<T>> schedule(const std::function<T()>& task, uint64_t delay_ms) {
            auto p = std::make_shared<std::promise<T>>();
            if (!p) {
                return std::nullopt;
            }

            auto scheduled_time = std::chrono::system_clock::now() + std::chrono::milliseconds(delay_ms);
            return submit_inner<T>(p, [p, task, scheduled_time]() {
                std::this_thread::sleep_until(scheduled_time);
                T res = task();
                p->set_value(std::move(res));
            });
        }

        template<class T, class U = typename std::enable_if<std::is_same<T, void>::value>::type>
        std::optional<std::future<void>> schedule(const std::function<void()>& task, uint64_t delay_ms) {
            auto p = std::make_shared<std::promise<void>>();
            if (!p) {
                return std::nullopt;
            }

            auto scheduled_time = std::chrono::system_clock::now() + std::chrono::milliseconds(delay_ms);
            return submit_inner<void>(p, [p, task, scheduled_time]() {
                std::this_thread::sleep_until(scheduled_time);
                task();
                p->set_value();
            });
        }

        void shutdown() {
            {
                auto lock = std::unique_lock(mutex_);
                if (!active_) {
                    return;
                }
                active_ = false;
            }

            cv_.notify_all();
            for (auto& t : threads_) {
                if (t.joinable()) {
                    t.join();
                }
            }
        }

        bool is_shutdown() {
            auto lock = std::shared_lock(mutex_);
            return !active_;
        }

    private:
        template<class T>
        std::optional<std::future<T>> submit_inner(const std::shared_ptr<std::promise<T>>& p, std::function<void()>&& task_wrapper) {
            {
                auto lock = std::unique_lock(mutex_);
                if (!active_) {
                    return std::nullopt;
                }

                queue_.push(std::move(task_wrapper));
            }

            cv_.notify_one();

            return p->get_future();
        }

    private:
        mutable std::shared_mutex mutex_;
        std::condition_variable_any cv_;
        std::queue<std::function<void()>> queue_;
        std::vector<std::thread> threads_;
        bool active_;
};
using thread_pool = ThreadPool;
};

#endif // THREAD_EXT_THREAD_POOL_H_
