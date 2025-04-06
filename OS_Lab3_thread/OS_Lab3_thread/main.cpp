#include "marker_thread.h"

int main() {
    int array_size;
    std::cout << "Enter array size: ";
    std::cin >> array_size;

    SharedData data;
    data.array.resize(array_size, 0);

    int num_threads;
    std::cout << "Enter number of marker threads: ";
    std::cin >> num_threads;

    data.active_threads = num_threads;
    data.shutdown_flags.resize(num_threads, false);
    data.proceed.resize(num_threads, false);

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) 
    {
        threads.emplace_back(marker_thread, i, std::ref(data));
    }

    while (data.active_threads > 0) {
        data.cv_marker.notify_all();
        {
            std::unique_lock<std::mutex> lock(data.mut);
            data.cv_main.wait(lock, [&data]() {
                return data.blocked_threads == data.active_threads;
                });
        }

        std::cout << "Current array state:" << std::endl;
        {
            std::lock_guard<std::mutex> lock(data.mut);
            for (int val : data.array) {
                std::cout << val << " ";
            }
        }
        std::cout << std::endl;

        int thread_to_terminate;
        std::cout << "Enter thread number to terminate: ";
        std::cin >> thread_to_terminate;
        thread_to_terminate--;

        while (thread_to_terminate < 0 || thread_to_terminate >= num_threads || data.shutdown_flags[thread_to_terminate])
        {
            std::cout << "Thread doesn't exist. Enter thread number to terminate again: ";
            std::cin >> thread_to_terminate;
            thread_to_terminate--;
        }

        {
            std::lock_guard<std::mutex> lock(data.mut);
            data.shutdown_flags[thread_to_terminate] = true;
        }
        data.cv_marker.notify_all();

        if (threads[thread_to_terminate].joinable()) {
            threads[thread_to_terminate].join();
        }

        std::cout << "Array after terminating thread " << (thread_to_terminate + 1) << ":" << std::endl;
        {
            std::lock_guard<std::mutex> lock(data.mut);
            for (int val : data.array) {
                std::cout << val << " ";
            }
        }
        std::cout << std::endl; 

        if (data.active_threads > 0) {
            std::lock_guard<std::mutex> lock(data.mut);
            data.proceed.clear();
            data.proceed.resize(num_threads, true);
            data.blocked_threads = 0;
        }
    }
    system("pause");
    return 0;
}