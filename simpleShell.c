#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>


char storeHistory[100][100];
int count = 0;

void addHistory(char* command) {
    if (count<=100) {
        strcpy(storeHistory[count], command);
        count++;
    }

    else {
        for(int i =1; i<100; i++) {
            strcpy(storeHistory[i-1],storeHistory[i]);
        }

        strcpy(storeHistory[99],command);
    }
}

void showHistory() {
    for (int i=0 ; i<count ; i++) {
        printf("%d) %s\n",i+1,storeHistory[i]);
    }
}

// Function to read user input and tokenize it
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
    char* token = strtok(line, " ");  // creating an array of strings by splitting the command by spaces
    while (token != NULL) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL; // Null-terminate the argument list

    return i; // Return the number of arguments
}


// Function to create a child process and run a command
int create_process_and_run(char* args[]) {
    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid < 0) {
        perror("Fork error");
        return 1;
    } 
    
    else if (pid == 0) { 
        // Child process
        // Execute the command in the child process
        if (execvp(args[0], args) == -1) {
            perror("Command execution error");
            exit(EXIT_FAILURE);
        }
    } 
    
    else {
        // Parent process
        waitpid(pid, NULL, 0);
        printf("Child process (PID %d) terminated\n", pid);
    }

    return 1; // Continue the shell loop
}


int change_directory(char* directory) { // change the directory 
    if (chdir(directory) == 0) {
        //printf("Changed directory to: %s\n", directory);
        return 1;
    } else {
        perror("cd error");
        return 1;
    }
}

int launch(char* command) {
    char* args[100]; 
    int num_args, status;

    num_args = read_input(command, args); // number of arguments
    addHistory(command);

    // Check if the command is "cd"
    if (strcmp(args[0], "cd") == 0) {
        if (num_args != 2) {
            printf("missing arguments\n");
        } else {
            status = change_directory(args[1]);
        }
    }

    else if (strcmp(args[0], "history") == 0) {
        //printf("%d\n",count);
        showHistory();
        status = 1;
    }

    else if(strcmp(args[0], "exit") == 0) {
        status = 0; //terminate shell
    }


    else {
        status = create_process_and_run(args); // for all commands other than cd
    }

    return status;
}

// Main shell loop
void shell_loop() {
    char input[100];
    int status;

    do {
        printf("YoyoShell$ ");
        status = launch(input);
        free(line);
    } while (status);
}

int main() {
    shell_loop();
    return EXIT_SUCCESS;    
}


// char *read_user_input();

// void shell_loop(){
//     int status;
//     do{
//         printf("> ");
//         char *line = read_user_input();
//         status= launch(line);
//         free(line);
//     }while(status);
// }

// char *read_user_input(){
//     char *line = NULL;
//     ssize_t bufsize = 0;
//     getline(&line, &bufsize, stdin);
//     return line;
// }

// int launch(char *args){
//     int status;
//     status= create_process_and_run(args);
//     return status;
// }

// int create_process_and_run(char *command){
//     int status = fork();
//     if(status == 0){
//         char *argv[3];
//         argv[0] = "/bin/sh";
//         argv[1] = "-c";
//         argv[2] = command;
//         argv[3] = NULL;
//         execv("/bin/sh", argv);
//         exit(EXIT_FAILURE);
//     }else if(status < 0){
//         perror("Error forking");

//     }else{
//         waitpid(status, NULL, 0);
//     }
// }

// int main(int argc, char **argv){
//     shell_loop();
//     return EXIT_SUCCESS;    
// }