#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>


const int SIZE = 100;    // Adjust the size of your task

typedef struct {
    int low;
    int high;
    int id;
    std::function<void(int)> lambda;
} thread_args;

void *thread_func(void *arg) {
    thread_args *args = (thread_args *)arg;

    // Your logic for dividing the loop range and running the lambda in parallel goes here
    for (int i = args->low; i < args->high; ++i) {
        args->lambda(i);
    }

    return NULL;
}

void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads){
    pthread_t tid[numThreads];
    thread_args args[numThreads];
    int chunk = SIZE / numThreads;
    for (int i = 0; i < numThreads; i++) {
        args[i].low = i * chunk;
        if (i==numThreads-1) {
            args[i].high = SIZE;
        }
        else {
            args[i].high = (i + 1) * chunk;
        }

        pthread_create(&tid[i], NULL, thread_func, (void *)&args[i]);
    }
}

void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> &&lambda, int numThreads){
    pthread_t tid[numThreads];
    thread_args args[numThreads];
    int chunk = SIZE / numThreads;
    }



void demonstration(std::function<void(int)> &&lambda) {
    // You can use this function to demonstrate the lambda functionality
    lambda(0);  // Pass a sample parameter to the lambda
}

int user_main(int argc, char **argv) {
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
