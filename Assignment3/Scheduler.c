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
#include <time.h>

int ncpu;
int tslice;
int ready_count = 0;
int running_count = 0;

// Define a structure to represent a process
typedef struct {
    pid_t pid;
    int priority;
    int burst_time;
    time_t start_time;
    time_t end_time;
} Process;

const char *name = "OS";
const char *name1 = "pidAndPriority";

Process ready_processes[100];
Process running_processes[100];
int total_processes = 0;

// Create a priority queue structure
typedef struct {
    Process array[100];
    int size;
} PriorityQueue;

PriorityQueue createPriorityQueue() {
    PriorityQueue pq;
    pq.size = 0;
    return pq;
}

// Add a process to the priority queue
void insert(PriorityQueue *pq, Process process) {
    if (pq->size < 100) {
        pq->array[pq->size] = process;
        pq->size++;

        int current = pq->size - 1;
        while (current > 0) {
            int parent = (current - 1) / 2;
            if (pq->array[current].priority < pq->array[parent].priority) {
                // Swap the process with its parent
                Process temp = pq->array[current];
                pq->array[current] = pq->array[parent];
                pq->array[parent] = temp;
                current = parent;
            } else {
                break;
            }
        }
    }
}

// Remove and return the process with the highest priority
Process extractMin(PriorityQueue *pq) {
    if (pq->size == 0) {
        Process empty = {0, 0, 0};
        return empty;
    }

    Process min = pq->array[0];
    pq->size--;

    if (pq->size > 0) {
        pq->array[0] = pq->array[pq->size];
        int current = 0;

        while (1) {
            int left = 2 * current + 1;
            int right = 2 * current + 2;
            int smallest = current;

            if (left < pq->size && pq->array[left].priority < pq->array[smallest].priority) {
                smallest = left;
            }

            if (right < pq->size && pq->array[right].priority < pq->array[smallest].priority) {
                smallest = right;
            }

            if (smallest != current) {
                // Swap the process with the smallest priority child
                Process temp = pq->array[current];
                pq->array[current] = pq->array[smallest];
                pq->array[smallest] = temp;
                current = smallest;
            } else {
                break;
            }
        }
    }

    return min;
}

void add_process_to_ready_queue(pid_t pid, int priority, int burst_time) {
    if (ready_count < 100) {
        Process process = {pid, priority, burst_time, 0, 0}; // Initialize start and end times
        insert(&ready_processes, process);
        ready_count++;
    }
}

void schedule() {
    // You need to define the sigchld_handler function here or remove this line if not used
    // signal(SIGCHLD, sigchld_handler);

    while (total_processes > 0) {
        // Move any ready processes to the running state
        while (ready_count > 0 && running_count < ncpu) {
            Process next_process = extractMin(&ready_processes);

            if (next_process.pid == 0) {
                break;
            }

            // Start the process on the first available CPU
            for (int i = 0; i < ncpu; i++) {
                if (running_processes[i].pid == 0) {
                    next_process.start_time = time(NULL); // Record start time
                    running_processes[i] = next_process;
                    running_count++;
                    break;
                }
            }

            // Signal the process to start execution
            kill(next_process.pid, SIGCONT);
        }

        // Sleep for the time quantum (tslice)
        usleep(tslice);

        // Check and reschedule processes with burst times greater than tslice
        for (int i = 0; i < ncpu; i++) {
            if (running_processes[i].pid != 0) {
                // Send a SIGSTOP signal to pause the process
                kill(running_processes[i].pid, SIGSTOP);

                // Check if the process has remaining burst time
                if (running_processes[i].burst_time > tslice) {
                    // Subtract tslice from burst time
                    running_processes[i].burst_time -= tslice;

                    // Add the process back to the ready queue
                    insert(&ready_processes, running_processes[i]);
                    ready_count--;
                } else {
                    // Process completed, decrement the total process count
                    total_processes--;
                    running_processes[i].end_time = time(NULL); // Record end time
                }

                // Clear the running process
                running_processes[i].pid = 0;
                running_count--;
            }
        }
    }

    // Print the PID, total execution time, and wait time for each process
    for (int i = 0; i < ncpu; i++) {
        if (running_processes[i].pid != 0) {
            time_t execution_time = running_processes[i].end_time - running_processes[i].start_time;
            time_t wait_time = execution_time - running_processes[i].burst_time;

            printf("PID: %d\n Execution Time: %ld seconds\n Wait Time: %ld seconds\n", running_processes[i].pid, execution_time, wait_time);
        }
    }
}

struct ProcessInfo {
    int value;
    pid_t pid;
};

int main(int argc, char *argv[]) {
    // Create and access shared memory to read ncpu and tslice values
    const int SIZE = 4096;
    int shm_fd1, shm_fd;
    void *ptr1;
    void *ptr;

    // Replace SHARED_MEM_NAME with the actual shared memory name
    shm_fd1 = shm_open(name, O_RDONLY, 0666);
    ptr1 = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd1, 0);

    // Read ncpu and tslice values from shared memory
    memcpy(&ncpu, ptr1, sizeof(int));
    memcpy(&tslice, ptr1 + sizeof(int), sizeof(int));

    printf("%d, %d\n", ncpu, tslice);

    //creating shared memory for pid and priority
    shm_fd = shm_open(name1, O_RDONLY, 0);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ProcessInfo processInfo;
    memcpy(&processInfo, ptr, sizeof(struct ProcessInfo));
    printf("Value: %d\n", processInfo.value);
    printf("PID: %d\n", processInfo.pid);

    int burst_time; // Set the burst time appropriately
    createPriorityQueue(); // You don't need to reassign to ready_processes
    add_process_to_ready_queue(processInfo.pid, processInfo.value, burst_time);

    if (fork() == 0) {
        schedule();
    }

    // SimpleShell executable files

    return 0;
}
