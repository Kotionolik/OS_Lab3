#include <gtest/gtest.h>
#include "marker_thread.h"

TEST(SharedDataTest, Initialization) {
    SharedData data;
    InitializeCriticalSection(&data.cs);
    data.main_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    data.thread_events.resize(3);
    for (auto& event : data.thread_events) {
        event = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    EXPECT_NE(data.main_event, nullptr);
    for (auto& event : data.thread_events) {
        EXPECT_NE(event, nullptr);
    }
    EXPECT_TRUE(data.array.empty());

    DeleteCriticalSection(&data.cs);
    CloseHandle(data.main_event);
    for (auto& event : data.thread_events) CloseHandle(event);
}

TEST(MarkerThreadTest, MarkingElements) {
    SharedData data;
    data.array.resize(10, 0);
    InitializeCriticalSection(&data.cs);
    data.main_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    data.thread_events.resize(1);
    data.thread_events[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    data.active_threads = 1;
    data.shutdown_flags.resize(1, false);
    data.proceed.resize(1, false);

    ThreadParams* params = new ThreadParams{ 0, &data };
    HANDLE hThread = CreateThread(NULL, 0, marker_thread, params, 0, NULL);

    SetEvent(data.thread_events[0]);

    WaitForSingleObject(data.main_event, INFINITE);
    ResetEvent(data.main_event);

    EnterCriticalSection(&data.cs);
    int marked_count = 0;
    for (int val : data.array) if (val == 1) marked_count++;
    LeaveCriticalSection(&data.cs);

    EXPECT_GT(marked_count, 0);

    data.shutdown_flags[0] = true;
    SetEvent(data.thread_events[0]);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    DeleteCriticalSection(&data.cs);
}

TEST(MarkerThreadTest, ThreadTermination) {
    SharedData data;
    data.array.resize(5, 0);
    InitializeCriticalSection(&data.cs);
    data.main_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    data.thread_events.resize(1);
    data.thread_events[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    data.active_threads = 1;
    data.shutdown_flags.resize(1, false);
    data.proceed.resize(1, false);

    ThreadParams* params = new ThreadParams{ 0, &data };
    HANDLE hThread = CreateThread(NULL, 0, marker_thread, params, 0, NULL);

    SetEvent(data.thread_events[0]);
    WaitForSingleObject(data.main_event, INFINITE);
    ResetEvent(data.main_event);
    data.shutdown_flags[0] = true;
    SetEvent(data.thread_events[0]);
    WaitForSingleObject(hThread, INFINITE);

    EnterCriticalSection(&data.cs);
    for (int val : data.array) EXPECT_EQ(val, 0);
    EXPECT_EQ(data.active_threads, 0);
    LeaveCriticalSection(&data.cs);

    CloseHandle(hThread);
    DeleteCriticalSection(&data.cs);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}