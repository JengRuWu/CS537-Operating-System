#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#define delim " \t\n"


void msgError(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int main(int argc, char **argv){
if(argc>1){
    msgError();
    exit(1);
}

int cmdln_no = 0;
int outFile, inFile;
pid_t childpid[1000];
int childnum=0;

while(1){
    char buffer[1000];

    char *saveptr;
    cmdln_no ++;
    fprintf(stdout, "wish (%d)> ", cmdln_no );
    fflush(stdout);

    fgets(buffer, 1000, stdin);
    int i;
    if(strlen(buffer)>128){
        msgError();
        continue;
    }

    //Parse
    int index = 0;
    int countArg;
    char*listCmd[128];

    listCmd[index] = strtok_r(buffer, delim, &saveptr);
    while(listCmd[index]!=NULL){
        index++;
        listCmd[index] = strtok_r(NULL, delim, &saveptr);
    }
    countArg = index;
    listCmd[countArg] = NULL;

    if(listCmd[0] == NULL){
        cmdln_no --;
        continue;
    }

    //is_redirect = 1: output
    //is_redirect = 2: input
    //is_redirect = 3: input & output
    int redirect_pipe_err = 0;
    int is_out_redirect = -1;
    int is_in_redirect = -1;
    int is_pipe = 0;
    int is_bg = 0;
    
    for(i=0; i<countArg; i++){
        if(strcmp(listCmd[i], ">")==0){
            if(i!=(countArg-2) && strcmp(listCmd[i+2],"<")!=0 && strcmp(listCmd[i+2],"&")!=0){
                redirect_pipe_err = -1;
                break;
            }
            is_out_redirect = i;
        }else if(strcmp(listCmd[i], "<")==0){
            if(i!=(countArg-2) && strcmp(listCmd[i+2],">")!=0&& strcmp(listCmd[i+2],"&")!=0){
                redirect_pipe_err = -1;
                break;
            }
            is_in_redirect = i;
        }else if(strcmp(listCmd[i], "|")==0){
            if(listCmd[i+1]==NULL){
                redirect_pipe_err = -1;
                break;
            }
            is_pipe = i;
        }
        if(strcmp(listCmd[i], "&")==0 && i == (countArg-1)){
            is_bg = 1;
            listCmd[i] = NULL;
        }
    }

    if(redirect_pipe_err==-1){
        msgError();
        continue;
    }
    
    //Built-in
    if (strcmp(listCmd[0], "exit") == 0) {
        if(childnum>0){
            for(int h=0;h<childnum;h++){
                if(childpid[h]!=-1){
                    kill(childpid[h],SIGKILL);
                }
            }
        }
        exit(0);
    }else if(strcmp(listCmd[0], "pwd") == 0){
        if(listCmd[1]!=NULL){
            msgError();
            continue;
        }
        printf("%s\n", getcwd(buffer, 1024));
    }else if(strcmp(listCmd[0], "cd") == 0 ){
        if (listCmd[1] == NULL) {
          chdir(getenv("HOME"));
        } else {
          if (chdir(listCmd[1]) == -1) {
            msgError();
            continue;
          }
        }
    }else{
        //Fork
        int status;
        pid_t pid = fork();
        if(pid == 0){

            char*pipe1[is_pipe+1];
            char*pipe2[countArg-is_pipe];

                if (is_out_redirect > 0) {
                    //redirect output
                    outFile = open(listCmd[is_out_redirect+1], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
                    if(outFile<0){
                        msgError();
                        exit(1);
                    }
                    dup2(outFile, 1) ;
                    listCmd[is_out_redirect] = NULL;
                    listCmd[is_out_redirect+1] = NULL;

                }
                
                if(is_in_redirect>0){
                    //redirect input
                    inFile = open(listCmd[is_in_redirect+1], O_RDONLY, S_IRWXU);
                    if(inFile<0){
                        msgError();
                        exit(1);
                    }
                    dup2(inFile, 0) ;
                    listCmd[is_in_redirect] = NULL;
                    listCmd[is_in_redirect+1] = NULL;
                }

            if(is_pipe>0){
                //seperate the two process of pipe
                for (i = 0; i < is_pipe; i++) {
                    pipe1[i] = listCmd[i];
                }
                pipe1[is_pipe] = NULL;
                for (i = 0; i < countArg-is_pipe-1; i++) {
                    pipe2[i] = listCmd[i + is_pipe + 1];
                }
                pipe2[countArg-is_pipe-1] = NULL;
            }

            //if not piped run execvp
            if(is_pipe==0){
                if(execvp(listCmd[0], listCmd)==-1){
                    msgError();
                    exit(1);
                };
                exit(0);
            }else{
            //if piped 

                int pipedes[2];
                pid_t pipePid;
                pipe(pipedes);
                pipePid =fork(); //duplicate the child into two process
                if(pipePid==0){
                    //the child output into pipe
                    close(pipedes[0]);
                    dup2(pipedes[1], 1);
                    if(execvp(pipe1[0], pipe1)==-1){
                        msgError();
                        exit(1);
                    }
                    close(pipedes[1]);
                    exit(0);
                }else{
                    close(pipedes[1]);
                    dup2(pipedes[0], 0);
                    if(execvp(pipe2[0], pipe2)==-1){
                        msgError();
                        exit(1);
                    }
                    close(pipedes[0]);
                    kill(pipePid, SIGKILL);
                    exit(0);
                }
            }
            exit(0);
        }else{
            
            pid_t bgdie = waitpid(0, &status, WNOHANG);
            for(int k = 0;k<childnum;k++){
                if(childpid[k]==bgdie){
                    childpid[k] = -1;
                    break;
                }
            }
            if(is_bg!=0){
                childpid[childnum] = pid;
                childnum++;
            }else{
                waitpid(pid, &status, 0);
            }

        }
    }
}
close(outFile);
close(inFile);
exit(0);
}