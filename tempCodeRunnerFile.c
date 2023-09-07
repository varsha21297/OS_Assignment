#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>

void shell_loop(){
    int status;
    do{
        printf("> ");
        char *line = read_user_input();
        status= launch(line);
        free(line);
    }while(status);
}

char *read_user_input(){
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

int launch(char *args){
    int status;
    status= create_process_and_run(args);
    return status;
}

int create_process_and_run(char *command){
    int status = fork();
    if(status == 0){
        char *args[] = {command, NULL};
        execvp(args[0], args);
        exit(EXIT_FAILURE);
    }else if(status < 0){
        perror("Error forking");
    }else{
        wait(NULL);
    }   
    return 1;
}

int main(int argc, char **argv){
    shell_loop();
    return EXIT_SUCCESS;    
}