#include <iostream>
#include <list>
#include <functional>
#include <pthread.h>

const int NTHREADS = 4;  // Adjust the number of threads as needed
const int SIZE = 100;    // Adjust the size of your task

typedef struct {
    int low;
    int high;
    int id;
    int numThreads;
    std::function<void(int)> lambda;  // Use std::function for C++ lambda
} thread_args;

void *thread_func(void *arg) {
    thread_args *args = (thread_args *)arg;

    // Your logic for dividing the loop range and running the lambda in parallel goes here
    for (int i = args->low; i < args->high; ++i) {
        args->lambda(i);
    }

    return NULL;
}

void demonstration(std::function<void(int)> &&lambda) {
    // You can use this function to demonstrate the lambda functionality
    lambda(0);  // Pass a sample parameter to the lambda
}

int user_main(int argc, char **argv) {
    pthread_t tid[NTHREADS];
    thread_args args[NTHREADS];
    int chunk = SIZE / NTHREADS;

    for (int i = 0; i < NTHREADS; i++) {
        args[i].low = i * chunk;
        args[i].high = (i + 1) * chunk;
        args[i].numThreads = NTHREADS;
        args[i].lambda = [](int i) {
            // Your lambda functionality goes here
            std::cout << "Processing element: " << i << std::endl;
        };

        pthread_create(&tid[i], NULL, thread_func, (void *)&args[i]);
    }

    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    int x = 5, y = 1;

    auto lambda1 = [x, &y](int) {
        y = 5;
        std::cout << "====== Welcome to Assignment-" << y << " of the CSE231(A) ======" << std::endl;
    };

    demonstration(lambda1);

    // Your user_main logic goes here

    auto lambda2 = [](int) {
        std::cout << "====== Hope you enjoyed CSE231(A) ======" << std::endl;
    };

    demonstration(lambda2);

    return 0;
}

int main(int argc, char **argv) {
    return user_main(argc, argv);
}
