#ifndef SIMPLE_MULTITHREADER_H
#define SIMPLE_MULTITHREADER_H

#include <iostream>
#include <functional>
#include <pthread.h>
#include <chrono>

const int SIZE = 100;

typedef struct {
    int low;
    int high;
    std::function<void(int)> lambda;
} thread_args;

typedef struct {
    int low1;
    int high1;
    int low2;
    int high2;
    std::function<void(int, int)> lambda;
} thread_args_2D;

std::chrono::high_resolution_clock::time_point get_current_time() {
    return std::chrono::high_resolution_clock::now();
}

double calculate_elapsed_time(std::chrono::high_resolution_clock::time_point start,
                               std::chrono::high_resolution_clock::time_point end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void *thread_func(void *arg) {
    auto start_time = get_current_time();
    thread_args *args = static_cast<thread_args *>(arg);
    for (int i = args->low; i < args->high; i++) {
        args->lambda(i);
    }

    auto end_time = get_current_time();
    std::cout << "Thread 1D Elapsed Time: " << calculate_elapsed_time(start_time, end_time) << " ms\n";
    return nullptr;
}

void *thread_func_2D(void *arg) {
    auto start_time = get_current_time();
    thread_args_2D *args = static_cast<thread_args_2D *>(arg);

    for (int i = args->low1; i <= args->high1; i++) {
        for (int j = args->low2; j <= args->high2; j++) {
            args->lambda(i, j);
        }
    }

    auto end_time = get_current_time();
    std::cout << "Thread 2D Elapsed Time: " << calculate_elapsed_time(start_time, end_time) << " ms\n";
    return nullptr;
}

void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads) {
    pthread_t tid[numThreads];
    thread_args args[numThreads];
    int chunk = SIZE / numThreads;
    for (int i = 0; i < numThreads; i++) {
        args[i].low = i * chunk;
        args[i].high = (i == numThreads - 1) ? SIZE : (i + 1) * chunk;
        args[i].lambda = lambda;  // Capture lambda by value
        pthread_create(&tid[i], nullptr, thread_func, &args[i]);
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(tid[i], nullptr);
    }
}

void parallel_for(int low1, int high1, int low2, int high2,
                  std::function<void(int, int)> &&lambda, int numThreads) {
    int chunkSize1 = SIZE / numThreads;
    pthread_t threads[numThreads];

    for (int i = 0; i < numThreads; ++i) {
        int threadLow1 = low1 + i * chunkSize1;
        int threadHigh1 = (i == numThreads - 1) ? high1 : threadLow1 + chunkSize1 - 1;

        pthread_create(&threads[i], nullptr, &thread_func_2D,
                       new thread_args_2D{threadLow1, threadHigh1, low2, high2, lambda});  // Capture lambda by value
    }

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }
}

#endif // SIMPLE_MULTITHREADER_H
