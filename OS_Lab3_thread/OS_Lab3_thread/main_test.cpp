#include <gtest/gtest.h>
#include "marker_thread.h"

TEST(SharedDataTest, Initialization) {
    SharedData data;
    data.array.resize(10, 0);
    ASSERT_EQ(data.array.size(), 10);
    for (int val : data.array) {
        EXPECT_EQ(val, 0);
    }
}

TEST(MarkerThreadTest, MarkAndTerminate) {
    SharedData data;
    data.array.resize(5, 0);
    data.active_threads = 1;
    data.shutdown_flags.push_back(false);
    data.proceed.push_back(false);

    std::thread t(marker_thread, 0, std::ref(data));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        std::lock_guard<std::mutex> lock(data.mut);
        data.shutdown_flags[0] = true;
    }
    data.cv_marker.notify_all();
    t.join();
    {
        std::lock_guard<std::mutex> lock(data.mut);
        data.proceed[0] = true;
    }
    data.cv_marker.notify_all();

    for (int val : data.array) {
        EXPECT_EQ(val, 0);
    }
}
TEST(MultipleMarkerThreadTest, MulMarkAndTerminate) {
    SharedData data;
    data.array.resize(5, 0);
    data.active_threads = 2;
    data.shutdown_flags.resize(2, false);
    data.proceed.resize(2, false);

    std::vector<std::thread> threads;
    for (int i = 0; i < 2; ++i)
    {
        threads.emplace_back(marker_thread, i, std::ref(data));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        std::lock_guard<std::mutex> lock(data.mut);
        data.shutdown_flags[0] = true;
    }
    data.cv_marker.notify_all();
    threads[0].join();
    {
        std::lock_guard<std::mutex> lock(data.mut);
        data.proceed[0] = true;
        data.proceed[1] = true;
    }
    data.cv_marker.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        std::lock_guard<std::mutex> lock(data.mut);
        data.shutdown_flags[1] = true;
    }
    data.cv_marker.notify_all();
    threads[1].join();
    {
        std::lock_guard<std::mutex> lock(data.mut);
        data.proceed[0] = true;
        data.proceed[1] = true;
    }
    data.cv_marker.notify_all();

    for (int val : data.array) {
        EXPECT_EQ(val, 0);
    }
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}