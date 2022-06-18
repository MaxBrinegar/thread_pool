#include <gtest/gtest.h>
#include <future>
#include <iostream>
#include <memory>
#include <atomic>
#include <vector>
#include "thread_pool.h"

TEST(ThreadPoolTest, VoidTypeTaskInvoked) {
    auto tp = thread_ext::ThreadPool(1);
    uint8_t call_count = 0;
    auto task = [&call_count]() {
        ++call_count;
    };

    auto res = tp.submit<void>(task);
    EXPECT_NE(res, std::nullopt);

    res->get();

    EXPECT_EQ(call_count, 1);

    tp.shutdown();
    EXPECT_TRUE(tp.is_shutdown());
}

TEST(ThreadPoolTest, IntTypeTaskInvoked) {
    auto tp = thread_ext::ThreadPool(1);
    uint8_t call_count = 0;
    auto task = [&call_count]() {
        ++call_count;
        return 5;
    };

    auto res = tp.submit<uint8_t>(task);
    EXPECT_NE(res, std::nullopt);

    uint8_t n = res->get();
    EXPECT_EQ(n, 5);

    EXPECT_EQ(call_count, 1);

    tp.shutdown();
    EXPECT_TRUE(tp.is_shutdown());
}

TEST(ThreadPoolTest, IntTypeScheduledTaskInvoked) {
    auto tp = thread_ext::ThreadPool(1);
    uint8_t call_count = 0;
    auto task = [&call_count]() {
        ++call_count;
        return 1;
    };

    auto res = tp.schedule<uint8_t>(task, 10);
    EXPECT_NE(res, std::nullopt);

    uint8_t n = res->get();
    EXPECT_EQ(n, 1);

    EXPECT_EQ(call_count, 1);

    tp.shutdown();
    EXPECT_TRUE(tp.is_shutdown());
}

TEST(ThreadPoolTest, VoidTypeScheduledTaskInvoked) {
    auto tp = thread_ext::ThreadPool(1);
    uint8_t call_count = 0;
    auto task = [&call_count]() {
        ++call_count;
    };

    auto res = tp.schedule<void>(task, 10);
    EXPECT_NE(res, std::nullopt);

    res->get();

    EXPECT_EQ(call_count, 1);

    tp.shutdown();
    EXPECT_TRUE(tp.is_shutdown());
}

TEST(ThreadPoolTest, VerifyNoSubmitAfterShutdown) {
    auto tp = thread_ext::thread_pool(1);
    tp.shutdown();
    EXPECT_TRUE(tp.is_shutdown());
    uint8_t call_count = 0;
    auto task = [&call_count]() {
        ++call_count;
    };

    auto res = tp.submit<void>(task);
    EXPECT_EQ(call_count, 0);
    EXPECT_EQ(res, std::nullopt);
}

TEST(ThreadPoolTest, VerifyNoScheduleAfterShutdown) {
    auto tp = thread_ext::thread_pool(1);
    tp.shutdown();
    EXPECT_TRUE(tp.is_shutdown());
    uint8_t call_count = 0;
    auto task = [&call_count]() {
        ++call_count;
    };

    auto res = tp.schedule<void>(task, 1000000000000);
    EXPECT_EQ(call_count, 0);
    EXPECT_EQ(res, std::nullopt);
}

TEST(ThreadPoolTest, VerifyIsShutdown) {
    auto tp = thread_ext::thread_pool(1);
    EXPECT_FALSE(tp.is_shutdown());
    tp.shutdown();
    EXPECT_TRUE(tp.is_shutdown());
}

TEST(ThreadPoolTest, VerifyMixedScenario) {
    auto tp = std::make_unique<thread_ext::thread_pool>(3);
    EXPECT_NE(tp, nullptr);
    auto call_count = std::atomic<uint8_t>(0);
    auto futures = std::vector<std::future<std::string>>();
    auto task = [&call_count]() {
        ++call_count;
        return std::string("task called");
    };

    for (uint8_t i = 0; i < 100; ++i) {
        std::optional<std::future<std::string>> opt = (i % 7 == 0)
            ? tp->schedule<std::string>(task, 10)
            : tp->submit<std::string>(task);

        EXPECT_NE(opt, std::nullopt);
        futures.push_back(std::move(*opt));
    }

    for (auto& f : futures) {
        auto s = f.get();
        EXPECT_EQ(s, std::string("task called"));
    }

    EXPECT_EQ(call_count, 100);

    tp->shutdown();
    EXPECT_TRUE(tp->is_shutdown());
}

TEST(ThreadPoolTest, VerifyManyTasksScenario) {
    auto tp = std::make_unique<thread_ext::thread_pool>(10);
    EXPECT_NE(tp, nullptr);
    auto call_count = std::atomic<uint16_t>(0);
    auto futures = std::vector<std::future<void>>();
    auto task = [&call_count]() {
        ++call_count;
    };

    for (uint16_t i = 0; i < 1000; ++i) {
        std::optional<std::future<void>> opt = tp->submit<void>(task);

        EXPECT_NE(opt, std::nullopt);
        futures.push_back(std::move(*opt));
    }

    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(call_count, 1000);
}