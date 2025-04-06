#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdlib>

struct SharedData {
    std::vector<int> array;
    CRITICAL_SECTION cs;
    std::vector<HANDLE> thread_events;
    HANDLE main_event;
    int blocked_threads = 0;
    std::vector<bool> proceed;
    std::vector<bool> shutdown_flags;
    int active_threads = 0;
};

struct ThreadParams {
    int id;
    SharedData* data;
};

DWORD WINAPI marker_thread(LPVOID lpParam) {
    ThreadParams* params = (ThreadParams*)lpParam;
    int id = params->id;
    SharedData* data = params->data;
    delete params;

    std::vector<int> marked_indices;
    srand(id + 1);

    while (true) {

        WaitForSingleObject(data->thread_events[id], INFINITE);
        EnterCriticalSection(&data->cs);

        if (data->shutdown_flags[id]) {
            for (int idx : marked_indices) data->array[idx] = 0;
            data->active_threads--;
            ResetEvent(data->thread_events[id]);
            LeaveCriticalSection(&data->cs);
            return 0;
        }

        int index = rand() % data->array.size();
        if (data->array[index] == 0) {
            Sleep(5);
            data->array[index] = id + 1;
            marked_indices.push_back(index);
            Sleep(5);
        }
        else {
            std::cout << "Marker " << (id + 1) << " marked " << marked_indices.size()
                << " elements. Cannot mark index " << index << std::endl;
            data->blocked_threads++;

            if (data->blocked_threads == data->active_threads) {
                SetEvent(data->main_event);
            }

            data->proceed[id] = false;
            ResetEvent(data->thread_events[id]);
        }

        LeaveCriticalSection(&data->cs);
    }
    return 0;
}