void shell_loop() {
    int status;
    do {
        printf("iiitd@possum:~$ ");
        char* command = read_user_input();
        status = launch(command);
    } while(status);
}



int launch (char *command) {
    int status;
    status = create_process_and_run(command);
    return status;
}

int create_process_and_run(char* command) {
    int status = fork();
    if(status < 0) {
        printf("fork error");
    } else if(status == 0) {
    
    } else {
    
    }
    return 0;
}