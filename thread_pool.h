#ifndef THREAD_EXT_THREADPOOL_H_
#define THREAD_EXT_THREADPOOL_H_

#include <thread>
#include <vector>
#include <future>
#include <chrono>
#include <mutex>
#include <queue>
#include <optional>
#include <type_traits>

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
                                std::unique_lock lock(tp_mutex_);
                                cv_.wait(lock, [this]() { return !tasks_.empty() || !active_; });
                                if (!active_) {
                                    return;
                                }

                                task = std::move(tasks_.front());
                                tasks_.pop();
                            }

                            task();
                        }
                    } });
                }
            }

            template<class T, class U = typename std::enable_if<!std::is_same<T, void>::value>::type>
            std::optional<std::future<T>> submit(const std::function<T()>& task) {
                auto p = std::make_shared<std::promise<T>>();
                if (!p) {
                    return std::nullopt;
                }

                return submit_inner<T>(p, [p, task]() {
                    T res = task();
                    p->set_value(res);
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
                    p->set_value(res);
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
                    std::unique_lock lock(tp_mutex_);
                    active_ = false;
                }

                cv_.notify_all();
                for (auto& t : threads_) {
                    if (t.joinable()) {
                        t.join();
                    }
                }
            }
        private:
            template<class T>
            std::optional<std::future<T>> submit_inner(const std::shared_ptr<std::promise<T>>& p, std::function<void()>&& task_wrapper) {
                {
                    std::unique_lock lock(tp_mutex_);
                    if (active_ == false) {
                        return std::nullopt;
                    }
                    tasks_.push(task_wrapper);
                }

                cv_.notify_one();

                return p->get_future();
            }
        private:
            mutable std::mutex tp_mutex_;
            std::condition_variable cv_;
            std::queue<std::function<void()>> tasks_;
            std::vector<std::thread> threads_;
            bool active_;
    };
};

#endif // THREAD_EXT_THREADPOOL_H_
