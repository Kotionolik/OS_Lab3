#include <windows.h>
#include <iostream>
#include <vector>
#include "marker_thread.h"

int main() {
    int array_size, num_threads;
    std::cout << "Enter array size: ";
    std::cin >> array_size;
    std::cout << "Enter number of marker threads: ";
    std::cin >> num_threads;

    SharedData data;
    data.array.resize(array_size, 0);
    data.proceed.resize(num_threads, false);
    data.shutdown_flags.resize(num_threads, false);
    data.active_threads = num_threads;
    data.thread_events.resize(num_threads);

    InitializeCriticalSection(&data.cs);
    data.main_event = CreateEvent(NULL, TRUE, FALSE, NULL);

    for (int i = 0; i < num_threads; ++i) {
        data.thread_events[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    std::vector<HANDLE> threads;
    for (int i = 0; i < num_threads; ++i) {
        ThreadParams* params = new ThreadParams{ i, &data };
        threads.push_back(CreateThread(NULL, 0, marker_thread, params, 0, NULL));
    }

    for (auto& event : data.thread_events) SetEvent(event);

    while (data.active_threads > 0) {
        WaitForSingleObject(data.main_event, INFINITE);
        ResetEvent(data.main_event);

        std::cout << "Current array state:" << std::endl;
        EnterCriticalSection(&data.cs);
        for (int val : data.array) std::cout << val << " ";
        std::cout << "\n";
        LeaveCriticalSection(&data.cs);

        int thread_to_terminate;
        do {
            std::cout << "Enter thread number to terminate (1-" << num_threads << "): ";
            std::cin >> thread_to_terminate;
            thread_to_terminate--;
        } while (thread_to_terminate < 0 || thread_to_terminate >= num_threads || data.shutdown_flags[thread_to_terminate]);

        EnterCriticalSection(&data.cs);
        data.shutdown_flags[thread_to_terminate] = true;
        SetEvent(data.thread_events[thread_to_terminate]);
        data.blocked_threads = 0;
        LeaveCriticalSection(&data.cs);

        WaitForSingleObject(threads[thread_to_terminate], INFINITE);
        CloseHandle(threads[thread_to_terminate]);

        std::cout << "Array after terminating thread " << (thread_to_terminate + 1) << ":" << std::endl;
        EnterCriticalSection(&data.cs);
        for (int val : data.array) std::cout << val << " ";
        std::cout << "\n";
        LeaveCriticalSection(&data.cs);

        EnterCriticalSection(&data.cs);
        for (int i = 0; i < num_threads; ++i) {
            if (!data.shutdown_flags[i] && !data.proceed[i]) {
                data.proceed[i] = true;
                SetEvent(data.thread_events[i]);
            }
        }
        LeaveCriticalSection(&data.cs);
    }

    DeleteCriticalSection(&data.cs);
    for (auto& event : data.thread_events) CloseHandle(event);
    CloseHandle(data.main_event);
    system("pause");
    return 0;
}