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
pid_t process[1000];

int ncpu;
int tslice;
int count=0;

void add(pid_t pid, pid_t process[]){
    process[count]=pid;
    count++;
}

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

void my_handler(int sig) { //function to handle the SIGINT signal
    printf("\n");
    //showPID();
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
        //showHistory(); // Show history
        status = 1;
    } else if (strcmp(args[0], "exit") == 0) {
        status = 0; // Terminate the shell
    } else if (strcmp(args[0], "submit")==0){
        submit(args);
        status=1;
    }
    return status;
}

int submit(char *args[]){
    pid_t pid, wpid;
    //int status;

    pid = fork(); // Create a child process

    if (pid < 0) {
        perror("Fork error");
        return 1;
    } else if (pid == 0) {
        struct sigaction sig;
        memset(&sig, 0, sizeof(sig));
        sig.sa_handler = my_handler;
        sigaction(SIGCHLD, &sig, NULL);
        add(getppid(),process);
        exit(0);
        // Child process
    } else {
        // Parent process
        //waitpid(pid, NULL, 0);
        printf("Child process (PID %d) terminated\n", pid);
    }

    return 1;
}

void scheduler(int tslice, pid_t process[]){
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = my_handler;
    sigaction(SIGCHLD, &sig, NULL);
    for (int i=0; i<count; i++){
        if (process[i]!=0){
            kill(process[i], SIGSTOP);
            sleep(tslice); 
            kill(process[i], SIGCONT);
        }

    }
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

    create_shared_memory();

    if (signal(SIGINT, my_handler) == SIG_ERR) { //registering the signal handler for SIGINT; if error occurs, print error message
        perror("Signal error");
        return 1;
    }
    shell_loop(); //call the shell loop
    return 0;
}