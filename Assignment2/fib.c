#include <stdio.h>

int fib(int n) {
    if (n == 0 || n == 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}


int main(int argc, char *argv[]){
    int n = 0;
    if (argc == 2) {
        n = atoi(argv[1]);
    }
    printf("fib(%d) = %d\n", n, fib(n));
    return 0;
}