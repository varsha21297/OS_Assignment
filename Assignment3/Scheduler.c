#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include "dummy_main.h"

int ncpu;
int tslice;
int ready_count = 0;
int running_count = 0;
pid_t ready_processes[MAX_PROCESSES];
pid_t running_processes[MAX_PROCESSES];
int priorities[MAX_PROCESSES];

const char* SHARED_MEM_NAME = "OS";

void sigchld_handler(int signo) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Process with PID pid has terminated
        // Decrement running_count and remove the terminated process from the array
        running_count--;
        for (int i = 0; i < ncpu; i++) {
            if (running_processes[i] == pid) {
                running_processes[i] = 0;
                break;
            }
        }
    }
}

void add_process_to_ready_queue(pid_t pid, int priority) {
    if (ready_count < MAX_PROCESSES) {
        // Insert the process in a sorted order based on priority
        int i;
        for (i = ready_count - 1; i >= 0 && priorities[i] < priority; i--) {
            ready_processes[i + 1] = ready_processes[i];
            priorities[i + 1] = priorities[i];
        }
        ready_processes[i + 1] = pid;
        priorities[i + 1] = priority;
        ready_count++;
    } 
}

void schedule() {
    signal(SIGCHLD, sigchld_handler);

    while (1) {
        // Move any ready processes to the running state
        while (ready_count > 0 && running_count < ncpu) {
            pid_t next_process = ready_processes[0];

            // Remove the selected process from the ready queue
            for (int i = 0; i < ready_count - 1; i++) {
                ready_processes[i] = ready_processes[i + 1];
                priorities[i] = priorities[i + 1];
            }
            ready_count--;

            // Start the process on the first available CPU
            for (int i = 0; i < ncpu; i++) {
                if (running_processes[i] == 0) {
                    running_processes[i] = next_process;
                    running_count++;
                    break;
                }
            }

            // Signal the process to start execution
            kill(next_process, SIGCONT);
        }

        // Sleep for the time quantum (tslice)
        usleep(tslice);

        // Stop running processes and move them to the ready queue
        for (int i = 0; i < ncpu; i++) {
            if (running_processes[i] != 0) {
                // Send a SIGSTOP signal to pause the process
                kill(running_processes[i], SIGSTOP);

                // Move the process back to the ready queue
                add_process_to_ready_queue(running_processes[i], priorities[i]);
                running_processes[i] = 0;
                running_count--;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Create and access shared memory to read ncpu and tslice values
    const int SIZE = 4096;
    int shm_fd;
    void* ptr;

    shm_fd = shm_open(SHARED_MEM_NAME, O_RDONLY, 0666);
    ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);

    // Read ncpu and tslice values from shared memory
    memcpy(&ncpu, ptr, sizeof(int));
    memcpy(&tslice, ptr + sizeof(int), sizeof(int));


    if (fork() == 0) {
        schedule();
    }

    //SimpleShell executable files

    return 0;
}
