## Simple Shell Project in C

A feature-rich simple shell project to mimic the Bash shell that supports commands with I/O, redirections, and pipelines, and handles errors for all supported features.  
input examples:
- basic commands with arguments  
eg. `ls -a`
- file I/O redirection:  
eg. `echo 123 > 1.txt`
- pipes:  
rg. `ls -a | grep unix`
- internal commands:  
eg. `pwd` and `cd`
- CTRL-C and CTRL-D
- background task  
eg. `/bin/ls | cat &`
- the command jobs that prints a list of background tasks together with their running states

 
