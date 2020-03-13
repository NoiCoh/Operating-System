#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void child_handler(){
    int status = -1;
    while(-1 != waitpid( -1, &status, WNOHANG )); // Wait for all procceses    
}
void ctrl_c_handler(){
}

// input - arglist and it's size. Output - if there is pipe in arglist, return it's place in arglist. Else return -1;
int get_pipe_location_in_arglist(int count, char** arglist) {
    for (int i=0; i < (count - 1); i++) {
        if (0 == strcmp(arglist[i], "|")) {
            arglist[i] = NULL;
            return i;
        }
    }
    return -1;
}

int process_arglist(int count, char** arglist) {
    int is_background_process = 0;
    int status     = -1;
    int pipe_location = get_pipe_location_in_arglist(count, arglist);
    int is_pipe = (-1 == pipe_location) ? 0 : 1;
    pid_t pid1;
    pid_t pid2;
    int fd[2];
    
    if( -1 == pipe(fd)){
        perror("pipe");
        exit(1);
	}
    
    if (0 == strcmp(arglist[count-1], "&")) {
        arglist[count-1] = NULL;
        is_background_process = 1;
    }
    
    pid1 = fork();
    if (pid1 < 0) { 
        printf("Error in fork.\n"); 
        exit(1); 
    }
    if(pid1 == 0) { // Child process
        if(is_pipe) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            if(execvp(arglist[0], arglist) < 0){
                fprintf(stderr,"Error in pipe execution.\n");
                exit(1);
            }
        } else { // not pipe
            if(is_background_process){
                signal(SIGINT, SIG_IGN);
            }
            if(execvp(arglist[0], arglist) < 0){
                fprintf(stderr,"Error in execution.\n");
                exit(1);
            }
        }
    } else { // Parent process
        if(is_pipe){
            pid2 = fork();
            if (pid2 < 0) { 
                printf("Error in fork.\n"); 
                exit(1); 
            }
            if(pid2 == 0) {
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                if(execvp(arglist[pipe_location+1], &arglist[pipe_location+1]) < 0){
                    fprintf(stderr,"Error in pipe execution.");
                    exit(1);
                }
            } else {
                close(fd[0]);
                close(fd[1]);
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
            }
        }
        else if (0 == is_background_process) {
            waitpid(pid1, &status, 0);
        }    
    }
    return 1; //Success
}

int prepare(void) {
  struct sigaction zombie_child_action;
  struct sigaction ctrl_c_action;
  
  memset(&zombie_child_action, 0, sizeof(zombie_child_action));
  memset(&ctrl_c_action, 0, sizeof(ctrl_c_action));
  
  zombie_child_action.sa_sigaction = child_handler;
  zombie_child_action.sa_flags = SA_SIGINFO | SA_RESTART;
  
  ctrl_c_action.sa_sigaction = ctrl_c_handler;
  ctrl_c_action.sa_flags = SA_SIGINFO | SA_RESTART;
  if( 0 != sigaction(SIGCHLD, &zombie_child_action, NULL)){
    fprintf(stderr,"Signal handle registration " "failed. %s\n",strerror(errno));
    return -1;
  }
  if( 0 != sigaction(SIGINT, &ctrl_c_action, NULL)){
    fprintf(stderr,"Signal handle registration " "failed. %s\n",strerror(errno));
    return -1;
  }
    return 0;
}

int finalize(void) {
    return 0;
}