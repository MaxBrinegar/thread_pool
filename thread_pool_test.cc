#include <gtest/gtest.h>
#include <future>
#include <iostream>
#include <memory>
#include <atomic>
#include <vector>
#include "thread_pool.h"

TEST(ThreadPoolTest, VoidTypeTaskInvoked) {
    auto tp = thread_ext::ThreadPool(1);
    uint8_t callCount = 0;
    auto task = [&callCount]() {
        ++callCount;
    };

    auto res = tp.submit<void>(task);
    EXPECT_NE(res, std::nullopt);

    res->get();

    EXPECT_EQ(callCount, 1);

    tp.shutdown();
}

TEST(ThreadPoolTest, IntTypeTaskInvoked) {
    auto tp = thread_ext::ThreadPool(1);
    uint8_t callCount = 0;
    auto task = [&callCount]() {
        ++callCount;
        return 5;
    };

    auto res = tp.submit<uint8_t>(task);
    EXPECT_NE(res, std::nullopt);

    uint8_t n = res->get();
    EXPECT_EQ(n, 5);

    EXPECT_EQ(callCount, 1);

    tp.shutdown();
}

TEST(ThreadPoolTest, IntTypeScheduledTaskInvoked) {
    auto tp = thread_ext::ThreadPool(1);
    uint8_t callCount = 0;
    auto task = [&callCount]() {
        ++callCount;
        return 1;
    };

    auto res = tp.schedule<uint8_t>(task, 10);
    EXPECT_NE(res, std::nullopt);

    uint8_t n = res->get();
    EXPECT_EQ(n, 1);

    EXPECT_EQ(callCount, 1);

    tp.shutdown();
}

TEST(ThreadPoolTest, VoidTypeScheduledTaskInvoked) {
    auto tp = thread_ext::ThreadPool(1);
    uint8_t callCount = 0;
    auto task = [&callCount]() {
        ++callCount;
    };

    auto res = tp.schedule<void>(task, 10);
    EXPECT_NE(res, std::nullopt);

    res->get();

    EXPECT_EQ(callCount, 1);

    tp.shutdown();
}

TEST(ThreadPoolTest, VerifyNoSubmitAfterShutdown) {
    auto tp = thread_ext::ThreadPool(1);
    tp.shutdown();
    uint8_t callCount = 0;
    auto task = [&callCount]() {
        ++callCount;
    };

    auto res = tp.submit<void>(task);
    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(res, std::nullopt);
}

TEST(ThreadPoolTest, VerifyNoScheduleAfterShutdown) {
    auto tp = thread_ext::ThreadPool(1);
    tp.shutdown();
    uint8_t callCount = 0;
    auto task = [&callCount]() {
        ++callCount;
    };

    auto res = tp.schedule<void>(task, 10000000000);
    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(res, std::nullopt);
}

TEST(ThreadPoolTest, VerifyComplexScenario) {
    auto tp = std::make_unique<thread_ext::ThreadPool>(3);
    EXPECT_NE(tp, nullptr);
    auto callCount = std::atomic<uint8_t>(0);
    auto futures = std::vector<std::future<std::string>>();
    auto task = [&callCount]() {
        ++callCount;
        return std::string("task called");
    };

    for (uint8_t i = 0; i < 10; ++i) {
        std::optional<std::future<std::string>> opt = (i & 1)
            ? tp->schedule<std::string>(task, 10)
            : tp->submit<std::string>(task);

        EXPECT_NE(opt, std::nullopt);
        futures.push_back(std::move(*opt));
    }

    for (auto& f : futures) {
        auto s = f.get();
        EXPECT_EQ(s, std::string("task called"));
    }

    EXPECT_EQ(callCount, 10);

    tp->shutdown();
}