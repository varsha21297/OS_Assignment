#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>

char *read_user_input(){
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

int create_process_and_run(char *command){
    int status = fork();
    if(status == 0){
        char *argv[3];
        argv[0] = "/bin/sh";
        argv[1] = "-c";
        argv[2] = command;
        argv[3] = NULL;
        execv("/bin/sh", argv);
        exit(EXIT_FAILURE);
    }else if(status < 0){
        perror("Error");

    }else{
        waitpid(status, NULL, 0);
    }
}

int launch(char *args){
    int status;
    status= create_process_and_run(args);
    return status;
}

void shell_loop(){
    int status;
    do{
        printf("> ");
        char *line = read_user_input();
        status= launch(line);
        free(line);
    }while(status);
}

int main(int argc, char **argv){
    shell_loop();
    return EXIT_SUCCESS;    
}