#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <errno.h>
#include <signal.h>

#define MAX_ARGS 50
#define MAX_WORDS 200
#define MAX_LINES 80
#define MAX_JOBS 20

struct procTable
{
    int ID;
    int status; // 1 for done, 0 for running
    int argc;
    char** cmd;
};

void exeLoop();
int readLine(char** args);
int execute(char** Args, int argCount, int j, int pipeCnt, pid_t pipe_pid[]);
void setArgs(int argc, char** Args, char **args);
int exePipe(char** Args, int argCount);
int pwdCmd();
int cdCmd(char **args, int argCnt);
int listJobs(int* job_num, struct procTable jobs[], int job_pid[]);
void inputHandler(int dummy);

#endif
