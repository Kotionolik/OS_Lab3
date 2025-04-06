#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>

struct SharedData {
    std::vector<int> array;
    std::mutex mut;
    std::condition_variable cv_main;
    std::condition_variable cv_marker;
    int blocked_threads = 0;
    std::vector<bool> proceed;
    std::vector<bool> shutdown_flags;
    int active_threads = 0;
};

void marker_thread(int id, SharedData& data) {
    std::unique_lock<std::mutex> lock(data.mut);
    std::vector<int> marked_indices;
    srand(id + 1);
    data.cv_marker.wait(lock, [] {return true; });

    while (true) {
        int index = rand() % data.array.size();

        if (data.array[index] == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            data.array[index] = id + 1;
            marked_indices.push_back(index);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        else {
            std::cout << "Marker " << (id + 1) << " marked " << marked_indices.size()
                << " elements. Cannot mark index " << index << std::endl;
            data.blocked_threads++;

            if (data.blocked_threads == data.active_threads) {
                data.cv_main.notify_one();
            }

            data.cv_marker.wait(lock, [&data, id]() {
                return data.shutdown_flags[id] || data.proceed[id];
                });

            if (!data.shutdown_flags[id]) {
                data.proceed[id] = false;
                continue;
            }

            for (int idx : marked_indices) {
                data.array[idx] = 0;
            }
            data.active_threads--;

            return;
        }
    }
}