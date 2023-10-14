#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>  
#include <sys/mman.h>  
#include <sys/stat.h>

char storeHistory[1000][100]; //array to store the history of commands
pid_t storepid[1000]; //array to store the PID of the commands
time_t total_time[1000]; //array to store the total time taken to execute the commands
time_t start_time[1000]; //array to store the start time of the commands
time_t end_time[1000]; //array to store the end time of the commands
int count = 0; //variable to keep track of the number of commands executed

int ncpu;
int tslice;

int create_shared_memory() {
    const int SIZE = 4096;
    const char* name = "OS";
    
    int shm_fd;
    void* ptr;
    
    // Create or open the shared memory object
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;  // Return an error code
    }

    // Set the size of the shared memory object
    if (ftruncate(shm_fd, SIZE) == -1) {
        perror("ftruncate");
        return 1;  // Return an error code
    }

    // Map the shared memory object into the address space
    ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;  // Return an error code
    }
    
    // Store NCPU and TSLICE in shared memory
    memcpy(ptr, &ncpu, sizeof(int));
    memcpy(ptr + sizeof(int), &tslice, sizeof(int));

    return 0;
}


void addHistory(char *command, pid_t pid) {
    if (count < 1000) { 
        strcpy(storeHistory[count], command);
        storepid[count] = pid;
        time(&start_time[count]); //get the start time
        count++;
    } else {
        for (int i = 1; i < 1000; i++) {
            strcpy(storeHistory[i - 1], storeHistory[i]); //shift the history by one position
            storepid[i - 1] = storepid[i]; //shift the PID by one position
        }

        strcpy(storeHistory[999], command); //add the command to the last position
        storepid[999] = pid; //add the command and PID to the last position
    }
}

void showHistory() {
    for (int i = 0; i < count; i++) {
        printf("%s\n", storeHistory[i]);
    }
}

void showPID() { //function to print the PID, command, time at which command was executed and time taken to execute the command
    for (int i = 0; i < count; i++) {
        printf("Command: %s\t", storeHistory[i]);
        printf("PID: %d\t", storepid[i]);
        printf("Time at which command was executed: %ld\t", start_time[i]);
        time(&end_time[i]); //get the end time
        total_time[i] = end_time[i] - start_time[i]; //calculate the total time
        printf("Time taken to execute the command: %ld\n", total_time[i]);
    }
}


void my_handler(int sig) { //function to handle the SIGINT signal
    printf("\n");
    showPID();
    exit(0);
}

// Function to read user input and split it by spaces
int read_input(char *line, char *args[]) {
    size_t buffer = 0;
    ssize_t read;

    read = getline(&line, &buffer, stdin); // read the input from the user

    if (read == -1) {
        perror("getline error");
        exit(EXIT_FAILURE);
    }

    // Remove the newline character
    if (line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }

    int i = 0;
    char *token = strtok(line, " ");
    while (token != NULL) { //tokenize the input by spaces
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;

    return i;
}

int execute_pipeline(char *pipeline[], int num_commands) {
    int status = 0;

    int pipes[num_commands - 1][2]; // Create an array of pipes

    for (int i = 0; i < num_commands; i++) {
        char *command_args[100];
        int num_args = 0;

        // Tokenize the individual command by spaces
        char *token = strtok(pipeline[i], " ");
        while (token != NULL) {

            //printf("%s\n",token);

            command_args[num_args] = token;
            token = strtok(NULL, " ");
            num_args++;
        }
        command_args[num_args] = NULL;

        // for (int i=0 ; i<num_args ; i++) {
        //     printf("%s\n",command_args[i]);
        // }

        if (i < num_commands - 1) {
            // If not the last command, create a pipe
            if (pipe(pipes[i]) == -1) {
                perror("Pipe creation failed");
                return 1;
            }
        }

        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("Fork failed");
            return 1;
        } else if (child_pid == 0) {
            // This is the child process

            // Redirect input from the previous command (if not the first command)
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
            }

            // Redirect output to the next command (if not the last command)
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][1]);
            }

            // Close all pipe file descriptors in child processes
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            execvp(command_args[0], command_args);
            perror("Command execution error");
            exit(EXIT_FAILURE);
        } else {
            // This is the parent process

            // Close pipe file descriptors in the parent process
            if (i > 0) {
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            // Wait for the child process to finish
            waitpid(child_pid, &status, 0);
            addHistory(pipeline[i],getpid());
        }
    }

    //return WEXITSTATUS(status);
    return 1;
}


// Function to create a child process and run a command
int create_process_and_run(char *args[]) {
    pid_t pid, wpid;
    int status;

    pid = fork(); // Create a child process

    if (pid < 0) {
        perror("Fork error");
        return 1;
    } else if (pid == 0) {
        // Child process
        // Execute the command in the child process
        if (execvp(args[0], args) == -1) {
            perror("Command execution error");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        waitpid(pid, NULL, 0); // Wait for the child process to finish
        addHistory(args[0], pid); // Add the command to history
        printf("Child process (PID %d) terminated\n", pid);
    }

    return 1;
}

int change_directory(char *directory) {
    if (chdir(directory) == 0) { // change directory
        return 1;
    } else {
        perror("cd error");
        return 1;
    }
}

void start_scheduler() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork error");
    } else if (pid == 0) {
        // Child process
        // Set the child process as a new session leader to run as a daemon
        if (setsid() == -1) {
            perror("setsid error");
            exit(EXIT_FAILURE);
        }

        // Close standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        // Create a new shared memory region and store ncpu and tslice
        if (create_shared_memory() == 1) {
            exit(EXIT_FAILURE);
        }

        // Execute the scheduler program
        char* args[] = {"./scheduler", NULL};
        execvp(args[0], args);
        perror("execvp error");

        exit(EXIT_FAILURE);
    } 

    else {
        // Parent process
        // Wait for the child process to start
        sleep(1);
    }
}

int submit(char *args[]){

    if (create_shared_memory() == 1) {
        return 1;
    }

    start_scheduler();

    pid_t pid, wpid;
    int status;

    pid = fork(); // Create a child process

    if (pid < 0) {
        perror("Fork error");
        return 1;
    } else if (pid == 0) {
        exit(0);
        // Child process
    } else {
        // Parent process
        //waitpid(pid, NULL, 0); // Wait for the child process to finish
        //addHistory(args[0], pid); // Add the command to history
        printf("Child process (PID %d) terminated\n", pid);
    }

    return 1;
}


int launch(char *command) {
    char *args[100];
    int num_args, status;

    num_args = read_input(command, args); // number of arguments

    char result[1000];
    strcpy(result, ""); // Initialize result to empty string

    for (int i = 0; args[i] != NULL; i++) {
        strcat(result, args[i]);
        if (args[i + 1] != NULL) { // If not the last argument, add a space
            strcat(result, " ");
        }
    }

    strcpy(command,result);
    //printf("%s\n",command); 

    if (strcmp(args[0], "cd") == 0) {
        if (num_args != 2) {
            printf("missing arguments\n");
        } 
        else {
            status = change_directory(args[1]);
        }
    } 
    else if (strcmp(args[0], "history") == 0) {
        showHistory(); // Show history
        status = 1;
    } 
    else if (strcmp(args[0], "exit") == 0) {
        status = 0; // Terminate the shell
    } 
    else if (strcmp(args[0], "submit") == 0) { 
        if (num_args==2 || num_args==3) {
            int priority = 1;
            if (num_args == 3) {
                int priority = atoi(args[2]);
            }

            status = submit(args);
        }
        else {
            printf("missing arguments\n");
        }
    } 
    else {
        // Check for pipes in args
        int hasPipe = 0;

        for (int i = 0; i < num_args; i++) {
            if (strcmp(args[i], "|") == 0) {
                hasPipe = 1;
                break;
            }
        }

        if (hasPipe) {
            char *pipeline[100];
            int pipeline_index = 0;

            // Tokenize the command by pipe using strtok
            char *token = strtok(command, "|");
            while (token != NULL) {
                //printf("%s\n",token);
                pipeline[pipeline_index] = token;
                pipeline_index++;
                token = strtok(NULL, "|");
            }
            pipeline[pipeline_index] = NULL;

            // Handle piped commands
            status = execute_pipeline(pipeline, pipeline_index);
        } else {
            // For regular commands, create a process and run
            status = create_process_and_run(args);
            //addHistory(args[0], getpid());
        }
    }


    return status;
}

void shell_loop() {
    char input[100];
    int status;

    do { //infinite loop untill exit command is given
        printf("Group72Shell$ ");
        status = launch(input);
    } while (status);
    //showHistory();
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ncpu> <tslice>\n", argv[0]);
        return 1;
    }

    int ncpu = atoi(argv[1]);
    int tslice = atoi(argv[2]);

    if (ncpu <= 0 || tslice <= 0) {
        printf("Please set valid values for ncpu and tslice.\n");
        return 1;
    }

    //create_shared_memory();  // Create and store NCPU and TSLICE in shared memory

    if (signal(SIGINT, my_handler) == SIG_ERR) { //registering the signal handler for SIGINT; if error occurs, print error message
        perror("Signal error");
        return 1;
    }
    shell_loop(); //call the shell loop
    return 0;
}