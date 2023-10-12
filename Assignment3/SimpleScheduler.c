#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

char storeHistory[1000][100]; //array to store the history of commands
pid_t storepid[1000]; //array to store the PID of the commands
time_t total_time[1000]; //array to store the total time taken to execute the commands
time_t start_time[1000]; //array to store the start time of the commands
time_t end_time[1000]; //array to store the end time of the commands
int count = 0; //variable to keep track of the number of commands executed

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



// Function to create a child process and run a command
/*int create_process_and_run(char *args[]) {
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
}*/

int change_directory(char *directory) {
    if (chdir(directory) == 0) { // change directory
        return 1;
    } else {
        perror("cd error");
        return 1;
    }
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
        } else {
            status = change_directory(args[1]);
        }
    } else if (strcmp(args[0], "history") == 0) {
        showHistory(); // Show history
        status = 1;
    } else if (strcmp(args[0], "exit") == 0) {
        status = 0; // Terminate the shell
    } else if (strcmp(args[0], "submit")==0){
        submit(args);
        status=1;
    }
    
    /*else {
        // For regular commands, create a process and run
        status = create_process_and_run(args);
        //addHistory(args[0], getpid());
    }*/


    return status;
}

int submit(char *args[]){
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
        addHistory(args[0], pid); // Add the command to history
        printf("Child process (PID %d) terminated\n", pid);
    }

    return 1;
}

void scheduler(){

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

    if (signal(SIGINT, my_handler) == SIG_ERR) { //registering the signal handler for SIGINT; if error occurs, print error message
        perror("Signal error");
        return 1;
    }
    shell_loop(); //call the shell loop
    return 0;
}
