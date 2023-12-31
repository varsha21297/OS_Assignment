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
    time_t start_time;
    time_t end_time;
} Process;

struct ProcessInfo {
    int value;
    pid_t pid;
    char executable_name[100]; 
};

const char* name = "OS";
const char* name1 = "pidAndPriority";

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
void insert(PriorityQueue* pq, Process process) {
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
Process extractMin(PriorityQueue* pq) {
    if (pq->size == 0) {
        Process empty = {0, 0};
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

void add_process_to_ready_queue(pid_t pid, int priority) {
    if (ready_count < 100) {
        Process process = {pid, priority, 0, 0}; // Initialize start and end times
        insert(&ready_processes, process);
        ready_count++;
    }
}

void check_process_termination(pid_t pid) {
    int status;
    int result = waitpid(pid, &status, WNOHANG); // Use WNOHANG to perform a non-blocking wait

    if (result == -1) {
        perror("waitpid");
        // Handle error
    } else if (result == 0) {
        // Process is still running within the time slice

    } else if (WIFEXITED(status)) {
        // Process has terminated normally
        int exit_status = WEXITSTATUS(status);
    }
}

void schedule() {
    signal(SIGCHLD, sigchld_handler);

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

        // Check and reschedule processes
        for (int i = 0; i < ncpu; i++) {
            if (running_processes[i].pid != 0) {
                // Send a SIGSTOP signal to pause the process
                kill(running_processes[i].pid, SIGSTOP);

                // Check if the process has terminated within the time slice
                check_process_termination(running_processes[i].pid);

                // Clear the running process
                running_processes[i].pid = 0;
                running_count--;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Create and access shared memory to read ncpu and tslice values
    const int SIZE = 4096;
    int shm_fd1, shm_fd;
    void* ptr1;
    void* ptr;

    shm_fd1 = shm_open(name, O_RDWR, 0666);
    ptr1 = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd1, 0);

    // Read ncpu and tslice values from shared memory
    memcpy(&ncpu, ptr1, sizeof(int));
    memcpy(&tslice, ptr1 + sizeof(int), sizeof(int));

    printf("%d, %d\n", ncpu, tslice);

    // Creating shared memory for pid and priority
    shm_fd = shm_open(name1, O_RDWR, 0);
    ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Read process information from shared memory
    struct ProcessInfo processInfo;
    memcpy(&processInfo, ptr, sizeof(struct ProcessInfo));
    printf("Value: %d\n", processInfo.value);
    printf("PID: %d\n", processInfo.pid);

    createPriorityQueue();
    add_process_to_ready_queue(processInfo.pid, processInfo.value);

    if (fork() == 0) {
        schedule();
    }


    return 0;
}
