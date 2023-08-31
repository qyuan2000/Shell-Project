#include "shell.h"

void exeLoop(){

    struct procTable jobs[MAX_JOBS];
    int job_num = 0;
    pid_t job_pids[MAX_JOBS];
    for(int i=0;i<MAX_JOBS;i++){
        jobs[i].cmd = (char**)malloc(sizeof(char*)*MAX_ARGS);
        for(int j=0; j<MAX_ARGS; j++){
            jobs[i].cmd[j] = malloc(MAX_ARGS*sizeof(char));
            memset(jobs[i].cmd[j], 0, MAX_ARGS);
        }
    }

    do{
        char **args = (char**)malloc(sizeof(char*)*MAX_ARGS);
        for(int i=0; i<MAX_ARGS; i++){
            args[i] = malloc(MAX_WORDS*sizeof(char));
            memset(args[i], 0, MAX_WORDS);
        }
        
        int argCount = readLine(args);

        if(strcmp(args[0], "exit") == 0){
            printf("exit\n");
            fflush(stdout);
            for(int i=0; i<MAX_ARGS; i++){
                free(args[i]);
            }
            free(args);
            break;
        }
        if(argCount == 0){
            printf("\n");
            printf("mumsh $ ");
            fflush(stdout);
            for(int i=0; i<MAX_ARGS; i++){
                free(args[i]);
            }
            free(args);
            continue;
        }
        // so confused about ctrl+d !!!!1
        
        if(strcmp(args[0], "cd")==0){
            cdCmd(args, argCount);
            for(int i=0; i<MAX_ARGS; i++){
                free(args[i]);
            }
            free(args);
            printf("mumsh $ ");
            fflush(stdout);
            continue;
        }

        if(strcmp(args[argCount-1], "&")==0){// background process
            job_num++;
            jobs[job_num].ID = job_num;
            jobs[job_num].status = 0;
            jobs[job_num].argc = argCount-1;

            printf("[%d] ", job_num);
            for(int i=0; i<argCount-1;i++){
                printf("%s ", args[i]);
                strcpy(jobs[job_num].cmd[i], args[i]);
            }

            printf("&\n");
            printf("mumsh $ ");
            fflush(stdout);

            argCount = argCount-1;

            char *Args[argCount+1];
            setArgs(argCount, Args, args);// parse the arguments

            job_pids[job_num] = fork();
            if(job_pids[job_num] == 0){
                int inFd = dup(STDIN_FILENO);
                int outFd = dup(STDOUT_FILENO);
                exePipe(Args, argCount);
                dup2(inFd,STDIN_FILENO);
                dup2(outFd,STDOUT_FILENO);
                exit(0);
            }
            
            for(int i=0; i<MAX_ARGS; i++){
                free(args[i]);
            }
            free(args);
            
            fflush(stdout);
            continue;
        }

        if(strcmp(args[0], "jobs")==0){
            listJobs(&job_num, jobs, job_pids);
            for(int i=0; i<MAX_ARGS; i++){
                free(args[i]);
            }
            free(args);
            printf("mumsh $ ");
            fflush(stdout);
            continue;
        }
            
        char *Args[argCount+1];
        setArgs(argCount, Args, args);// parse the arguments
        int inFd = dup(STDIN_FILENO);
        int outFd = dup(STDOUT_FILENO);
        exePipe(Args, argCount);
        dup2(inFd,STDIN_FILENO);
        dup2(outFd,STDOUT_FILENO);
        
        for(int i=0; i<MAX_ARGS; i++){
            free(args[i]);
        }
        free(args);
        printf("mumsh $ ");
	    fflush(stdout);
    }while(1);

    for(int i=0;i<MAX_JOBS;i++){
        for(int j =0; j<MAX_ARGS;j++){
            free(jobs[i].cmd[j]);
        }
        free(jobs[i].cmd);
    }
}

int exePipe(char** Args, int argCount){
    int pipe_index[MAX_JOBS];
    int pipeCnt = 0;
    int missing_prog = 0;
    for(int i=0; i<argCount; i++){
        if(strcmp(Args[i], "|") == 0){
            pipe_index[pipeCnt] = i;
            pipeCnt++;
            if(pipeCnt >1){
                if((i - pipe_index[pipeCnt-2]) == 1)
                    missing_prog = 1;
            } 
        }
    }
    
    if(pipeCnt == 0){//does not contain "pipe"
        pid_t pid[1] = {0};
        execute(Args, argCount, 0, 0, pid);
        return 0;
    }
    else if(pipe_index[0] == 0 || pipe_index[pipeCnt-1] == argCount-1 || missing_prog == 1)
    {// snytex error near "pipe"
        fprintf(stderr, "error: missing program\n");
        return -1;
    }
    else
    {// pipe
        //generate multiple parallel process
        int fd[pipeCnt][2];
        for(int i = 0; i < pipeCnt; i++)//open all the pipes
            pipe(fd[i]);//create a pipe between two adjacent process
        pid_t pid[MAX_LINES];// use constant to repress the cpplint warning
        for(int j = 0; j < pipeCnt+1; j++){// total cmd number is pipeCnt+1
            pid[j] = fork();
            if(pid[j] == -1)
                fprintf(stderr, "fail to fork %d\n", j);
            if(pid[j] == 0){//child process
                for(int i = 0; i<pipeCnt; i++){//close all irrelavent pipes
                    if(i != j && i != j-1){
                        close(fd[i][0]);
                        close(fd[i][1]);
                    }
                }

                if(j == 0){// first cmd in cascade pipes
                    char *leftCmd[pipe_index[0]+1];
                    for(int word=0; word<pipe_index[0]; word++){
                        leftCmd[word] = Args[word];
                    }
                    leftCmd[pipe_index[0]] = NULL;
                    //close(1);
                    dup2(fd[j][1], STDOUT_FILENO);
                    close(fd[j][0]);
                    close(fd[j][1]);
                    execute(leftCmd, pipe_index[0], 0, pipeCnt, pid);
                    exit(0);
                }
                else if (j == pipeCnt)
                {// last cmd in cascade pipe
                    int n = argCount-pipe_index[j-1];
                    char *leftCmd[n];
                    for(int word=0; word<n-1; word++){
                        leftCmd[word] = Args[pipe_index[j-1]+word+1];
                    }
                    leftCmd[n-1] = NULL;
                    //close(0);
                    dup2(fd[j-1][0], 0);
                    close(fd[j-1][1]);
                    close(fd[j-1][0]);
                    execute(leftCmd, n-1, pipeCnt, pipeCnt, pid);
                    exit(0);
                }
                else
                {
                    int n = pipe_index[j]-pipe_index[j-1];
                    char *leftCmd[n];
                    for(int word=0; word<n-1; word++){
                        leftCmd[word] = Args[pipe_index[j-1]+word+1];
                    }
                    leftCmd[n-1] = NULL;
                    close(fd[j-1][1]);
                    dup2(fd[j-1][0], 0);
                    close(fd[j-1][0]);
                    close(fd[j][0]);
                    dup2(fd[j][1], STDOUT_FILENO);
                    close(fd[j][1]);
                    execute(leftCmd, n-1, j, pipeCnt, pid);
                    exit(0);
                }
                
            }
            
        }
        for(int k = 0; k< pipeCnt; k++){
            close(fd[k][0]);
            close(fd[k][1]);
        }
        for(int k = 0; k < pipeCnt+1; k++){
            waitpid(pid[k], NULL, 0);
        }
        
    }
    fflush(stdout);
    return 1;
}


int execute(char** Args, int argCount, int j, int pipeCnt, pid_t pipe_pid[]){
    char *parse_Args[argCount+1];
    int n = 0;
    int in_fd, out_fd;
    int double_input = 0;
    int double_output = 0;

    pid_t pid;
    pid = fork();
    if(pid == 0){
        for(int i=0; i<argCount;i++){
            if(strcmp(Args[i],">")==0)
            {
                if(i<argCount-1 && (strcmp(Args[i+1], ">") == 0 || strcmp(Args[i+1], "<") == 0 || strcmp(Args[i+1], ">>") == 0)){
                    fprintf(stderr, "syntax error near unexpected token `%s'\n", Args[i+1]);
                    fflush(stdout);
                    exit(0);
                }
                if(i == argCount-1 && pipeCnt!=0 && j<pipeCnt){
                    fprintf(stderr, "syntax error near unexpected token `|'\n");
                    fflush(stdout);
                    exit(0);
                }
                if(argCount < 3){
                    fprintf(stderr, "error: missing program\n");
                    fflush(stdout);
                    exit(0);
                }
                if(double_output==1 || (pipeCnt!=0 && j<pipeCnt))
                {
                    fprintf(stderr, "error: duplicated output redirection\n");
                    fflush(stdout);
                    exit(0); 
                }
                close(1);
                out_fd=open(Args[i+1], O_WRONLY|O_CREAT|O_TRUNC,0666);
                double_output=1;
                if(out_fd==-1)
                {
                    fprintf(stderr, "%s: Permission denied\n",Args[i+1]);
                    fflush(stdout);
                    exit(0);  
                }
                i=i+1;
                dup2(out_fd, 1);
            }
            else if(strcmp(Args[i], ">>")==0)
            {
                if(i<argCount-1 && (strcmp(Args[i+1], ">") == 0 || strcmp(Args[i+1], "<") == 0 || strcmp(Args[i+1], ">>") == 0)){
                    fprintf(stderr, "syntax error near unexpected token `%s'\n", Args[i+1]);
                    fflush(stdout);
                    exit(0);
                }
                if(double_output==1|| (pipeCnt!=0 && j<pipeCnt))
                {
                    fprintf(stderr, "error: duplicated output redirection\n");
                    fflush(stdout);
                    exit(0);  
                }
                close(1);
                out_fd=open(Args[i+1], O_WRONLY|O_CREAT|O_APPEND,0666);
                double_output=1;
                if(out_fd==-1)
                {
                    fprintf(stderr, "%s: Permission denied\n",Args[i+1]);
                    fflush(stdout);
                    exit(0);  
                }
                i=i+1;
                dup2(out_fd, 1);
            }
            else if(strcmp(Args[i], "<")==0)
            {
                if(i<argCount-1 && (strcmp(Args[i+1], ">") == 0 || strcmp(Args[i+1], "<") == 0 || strcmp(Args[i+1], ">>") == 0)){
                    fprintf(stderr, "syntax error near unexpected token `%s'\n", Args[i+1]);
                    fflush(stdout);
                    exit(0);
                }
                if(double_input==1 || (pipeCnt!=0 && j>0))
                {
                    fprintf(stderr, "error: duplicated input redirection\n");
                    fflush(stdout);
                    if(pipeCnt!=0 && j>0) waitpid(pipe_pid[j-1],NULL,WNOHANG);
                    exit(0);  
                }
                close(0);
                in_fd=open(Args[i+1], O_RDONLY);
                double_input=1;
                if(in_fd==-1)
                {
                    fprintf(stderr,"%s: %s\n",Args[i+1],strerror(errno));
                    fflush(stdout);
                    exit(0);  
                }
                i=i+1;
                dup2(in_fd, 0);
            }
            else
            {
                if(strcmp(Args[i], "|quo") == 0){
                    parse_Args[n] = "|";
                }
                else if(strcmp(Args[i], ">quo") == 0)
                {
                    parse_Args[n] = ">";
                }
                else if(strcmp(Args[i], "<quo") == 0)
                {
                    parse_Args[n] = "<";
                }
                else
                {
                    parse_Args[n] = Args[i];
                }
                n++;
            }
        }
        parse_Args[n] = NULL;

        if(strcmp(parse_Args[0], "pwd") == 0){
			pwdCmd();
			exit(0);
		}

        if(execvp(parse_Args[0], parse_Args)<0)
        {
            fprintf(stderr,"%s: command not found\n", Args[0]);
            fflush(stdout);
            exit(0);
        }
        fflush(stdout);
    }
    else
    { 
       wait(NULL);
       return 0;
    }
    return 0;
}

int pwdCmd(){
	char result[255];
	if(!getcwd(result, 255)){
		return 1;
		printf("ERROR IN PWD\n");

	}
	printf("%s\n", result);
	return 0;
}


int cdCmd(char **args, int argCnt){
	if (argCnt != 2){
		printf("Missing or too many arguments!\n");
		return 1;
	}
	int ret = chdir(args[1]);
	if(ret == -1){
		printf("%s: No such file or directory\n", args[1]);
		return 1;
	}
	return 0;
}


int listJobs(int* job_num, struct procTable jobs[], int job_pid[]){
    for(int i=1;i< *job_num+1;i++){
        printf("[%d] ", jobs[i].ID);
        if(waitpid(job_pid[i] ,NULL,WNOHANG) == 0) 
            printf("running ");
        else
            printf("done ");
        for(int j=0;j<jobs[i].argc;j++){
            printf("%s ", jobs[i].cmd[j]);
        }
        printf("&\n");
    }
    return 0;
}
