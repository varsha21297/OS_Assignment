#include <stdio.h>

int fib(int n) {
    if (n == 0 || n == 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char**argv){
    int n = atoi(argv[1]);
    int result = fib(n);
    printf("%d\n", result); // Print the result to standard output

    return 0;
}