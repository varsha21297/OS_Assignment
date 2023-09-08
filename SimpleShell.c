#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char storeHistory[1000][100];
int count = 0;

void addHistory(char *command) {
    if (count < 1000) { 
        strcpy(storeHistory[count], command);
        count++;
    } else {
        for (int i = 1; i < 1000; i++) {
            strcpy(storeHistory[i - 1], storeHistory[i]);
        }

        strcpy(storeHistory[999], command);
    }
}

void showHistory() {
    for (int i = 0; i < count; i++) {
        printf("%s\n", storeHistory[i]);
    }
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

    num_args = read_input(command, args); // number of arguments
    addHistory(command);

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
    }

    return status;
}

void shell_loop() {
    char input[100];
    int status;

    do {
        printf("YoyoShell$ ");
        status = launch(input);
    } while (status);
}

int main() {
    shell_loop();
    return 0;
}
