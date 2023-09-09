#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

char storeHistory[1000][100];
pid_t storepid[1000];
time_t total_time[1000];
time_t start_time[1000];
time_t end_time[1000];
int count = 0;

void addHistory(char *command, pid_t pid) {
    if (count < 1000) { 
        strcpy(storeHistory[count], command);
        storepid[count] = pid;
        time(&start_time[count]);
        count++;
    } else {
        for (int i = 1; i < 1000; i++) {
            strcpy(storeHistory[i - 1], storeHistory[i]);
            storepid[i - 1] = storepid[i];
        }

        strcpy(storeHistory[999], command);
        storepid[999] = pid;
    }
}

void showHistory() {
    for (int i = 0; i < count; i++) {
        printf("%s\n", storeHistory[i]);
    }
}

void showPID() {
    for (int i = 0; i < count; i++) {
        printf("%d\t", storepid[i]);
        
        printf("Time at which command was executed: %ld\t", start_time[i]);
        time(&end_time[i]);
        total_time[i] = end_time[i] - start_time[i];
        printf("Time taken to execute the command: %ld\n", total_time[i]);
    }
}


void my_handler(int sig) {
    printf("\n");
    showPID();
    exit(0);
}

// Function to read user input and split it by spaces
int read_input(char *line, char *args[]) {
    size_t buffer = 0;
    ssize_t read;

    read = getline(&line, &buffer, stdin);

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
    while (token != NULL) {
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

    pid = fork();

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
        waitpid(pid, NULL, 0);
        addHistory(args[0], pid);
        printf("Child process (PID %d) terminated\n", pid);
    }

    return 1;
}

int change_directory(char *directory) {
    if (chdir(directory) == 0) {
        return 1;
    } else {
        perror("cd error");
        return 1;
    }
}

int launch(char *command) {
    char *args[100];
    int num_args, status;

    num_args = read_input(command, args);

    char result[1000];
    strcpy(result, "");

    for (int i = 0; args[i] != NULL; i++) {
        strcat(result, args[i]);
        if (args[i + 1] != NULL) {
            strcat(result, " ");
        }
    }

    strcpy(command,result);
    //printf("%s\n",command); 

    if (strcmp(args[0], "cd") == 0) {
        if (num_args != 2) {
            printf("missing arguments\n");
        } else {
            status = change_directory(args[1]);
        }
    } else if (strcmp(args[0], "history") == 0) {
        showHistory();
        status = 1;
    } else if (strcmp(args[0], "exit") == 0) {
        status = 0; // Terminate the shell
    } else {
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
    /*char *args[100];
    int num_args, status;

    num_args = read_input(command, args); // number of arguments
    //addHistory(command);

    if (strcmp(args[0], "cd") == 0) {
        if (num_args != 2) {
            printf("missing arguments\n");
        } else {
            status = change_directory(args[1]);
        }
    } else if (strcmp(args[0], "history") == 0) {
        showHistory();
        status = 1;
    } else if (strcmp(args[0], "exit") == 0) {
        status = 0; // terminate shell
    } else {
        status = create_process_and_run(args); // for all other commands
        //addHistory(args[0], getpid());
    }

    return status;*/
}

void shell_loop() {
    char input[100];
    int status;

    do {
        printf("YoyoShell$ ");
        status = launch(input);
    } while (status);
    //showHistory();
}

int main() {
    if (signal(SIGINT, my_handler) == SIG_ERR) {
        perror("Signal error");
        return 1;
    }
    shell_loop();
    return 0;
}
