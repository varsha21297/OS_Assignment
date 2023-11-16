#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h>


const int SIZE = 100;    // Adjust the size of your task
void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads);
int user_main(int argc, char **argv);

typedef struct {
    int low;
    int high;
    int id;
    std::function<void(int)> lambda;
} thread_args;

void *thread_func(void *arg){
    thread_args *args = (thread_args *)arg;
    for (int i = args->low; i < args->high; i++) {
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

    for (int i = 0; i < numThreads; i++) {
        pthread_join(tid[i], NULL);
    }
}


void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> &&lambda, int numThreads){
    pthread_t tid[numThreads];
    thread_args args[numThreads];
    int chunk = SIZE / numThreads;
    }



void demonstration(std::function<void()> &&lambda) {
    lambda();  // Call the lambda with no arguments
}


int main(int argc, char **argv) {
    /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto lambda1 = [x, &y]() {
        y = 5;
        std::cout << "====== Welcome to Assignment-" << y << " of the CSE231(A) ======\n";
    };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);
 
  auto lambda2 = []() {
        std::cout << "====== Hope you enjoyed CSE231(A) ======\n";
    };

  demonstration(lambda2);
  return rc;
}

#define main user_main