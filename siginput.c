#include "shell.h"
// this .c is to revise 'input.c' so that it can suit signals and quotes
static int print;
void inputHandler(int dummy) {
	if(print==0)
		printf("\n");
	else
		printf("\nmumsh $ ");
	fflush(stdout);
    dummy = 2;// to repress warning
}

//the function of this readLine
// 1. deal with ctrl c, ctrl d
// 2. take off all valid quotes
// 3. split re-direction operators
// 4. output '>' and readin next line when the cmd is not complete
// return value is the number of arguments of this cmd
// after this function, 'args' should be ready for 'exeloop()' 
int readLine(char** args){
    char line[MAX_WORDS];
    int double_quote = 0; //set value to 1 if there is an unfinished left quote, set to 0 if two sides complete
    int single_quote = 0;
    int input_num = 0; // <
    int output_num = 0; // > and >>
    int syntax_error = 0;
    int num_char = 0; //count char in line
    print = 1; // have no idea why....
	signal(SIGINT, inputHandler);

    char ch;
    // store a regular cmd line in 'line[]'
    while(1){
        ch = (char)fgetc(stdin);
        if(ch == '\n'){
            if(num_char == 0){// empty line
                return 0;
            }
            else if((line[num_char-1] == '>' || line[num_char-1] == '<' || line[num_char-1] == '|') && syntax_error == 0)
            {// unfinished re-direction, read next line
                printf("> ");
                line[num_char] = ' ';
                num_char++;
                fflush(stdout);
                continue;
            }
            else if(double_quote == 1 || single_quote == 1)
            {// unfinished quotes, read next line
                printf("> ");
                line[num_char] = '\n';
                num_char++;
                fflush(stdout);
                continue;
            }
            else
            {// regular finished line
                break;
            }
        }
        else if (ch == EOF)
        {
            if(num_char == 0){// ctrl d when empty cmd line
                args[0][0] = 'e';
                args[0][1] = 'x';
                args[0][2] = 'i';
                args[0][3] = 't';
                args[0][4] = '\0';
                return 1;
            }
            else if((ch = (char)fgetc(stdin)) != EOF)
            {// ctrl d when cmd not finished
                line[num_char] = ch;
                num_char++;
                continue;
            }
            else
            {// regular end of stdin
                break;
            }
        }
        else
        {
            if(ch == '\''){
                single_quote = (single_quote+1)%2;
            }
            if(ch == '\"'){
                double_quote = (double_quote+1)%2;
            }
            if(ch == '<' && single_quote != 1 && double_quote != 1){
                input_num++;
                output_num = 0;
            }
            if(ch == '>' && single_quote != 1 && double_quote != 1 && num_char>0 &&line[num_char-1] != '>'){
                output_num++;
                input_num = 0;
            }
            if(ch == '|' && single_quote != 1 && double_quote != 1){
                output_num = 0;
                input_num = 0;
            }
            if(output_num>1 || input_num>1) syntax_error = 1;
            //fprintf(stderr, "in %d out %d error %d\n", input_num, output_num, syntax_error);
            line[num_char] = ch;
            num_char++;
        }
    }
    // split re-direction and take away quotes
    single_quote = 0;
    double_quote = 0;
    int n = 0;
    int m = 0;
    int count_char = 0;
    while(count_char<num_char){// loop: deal with line[count_char]
        ch = line[count_char];
        count_char++;
            
        if(ch == ' '){
            if(single_quote == 1 || double_quote == 1){
                args[n][m] = ch;
                m++;
                continue;
            }
            if(strlen(args[n]) != 0 && count_char != num_char)//deal with multiple spaces
                n++;
            m = 0;
        }
        else
        {
            if(ch == '>' && single_quote != 1 && double_quote != 1){
                if(line[count_char] == '>'){// >>
                    count_char++;
                    if(m == 0 && line[count_char] == ' '){// 1 >> 2
                        args[n][m] = ch;
                        args[n][m+1] = ch;
                        continue;
                    }
                    else if(m == 0 && line[count_char] != ' ')
                    {
                        args[n][m] = ch;
                        args[n][m+1] = ch;
                        m = 0;
                        n++;
                    }
                    else if(m != 0 && line[count_char] == ' ')
                    {
                        n++;
                        m = 0;
                        args[n][m] = ch;
                        args[n][m+1] = ch;
                    }
                    else
                    {
                        n++;
                        m = 0;
                        args[n][0] = ch;
                        args[n][1] = ch;
                        n++;
                    }
                }
                else
                {// >
                    if(m == 0 && line[count_char] == ' '){
                        args[n][m] = ch;
                        continue;
                    }
                    else if(m == 0 && line[count_char] != ' ')
                    {
                        args[n][m] = ch;
                        n++;
                    }
                    else if(m != 0 && line[count_char] == ' ')
                    {
                        n++;
                        m = 0;
                        args[n][m] = ch;
                    }
                    else
                    {
                        n++;
                        m = 0;
                        args[n][m] = ch;
                        n++;
                    }
                }
            }
            else if(ch == '<' && single_quote != 1 && double_quote != 1)
            {
                if(m == 0 && line[count_char] == ' '){
                    args[n][m] = ch;
                    continue;
                }
                else if(m == 0 && line[count_char] != ' ')
                {
                    args[n][m] = ch;
                    n++;
                }
                else if(m != 0 && line[count_char] == ' ')
                {
                    n++;
                    m = 0;
                    args[n][m] = ch;
                }
                else
                {
                    n++;
                    m = 0;
                    args[n][m] = ch;
                    n++;
                }
            }
            else if(ch == '\'')
            {
                single_quote = (single_quote+1)%2;
                if(double_quote != 0){
                    args[n][m] = ch;
                    m++;
                }
                else if ((line[count_char] == '|' || line[count_char] == '>' || line[count_char] == '<') && line[count_char+1] == '\''&& (line[count_char+2] == ' ' || line[count_char+2] == '\n'))
                {
                    args[n][m] = line[count_char];
                    args[n][m+1] = 'q';
                    args[n][m+2] = 'u';
                    args[n][m+3] = 'o';
                    m = 0;
                    n++;
                }
                
            }
            else if(ch == '\"')
            {
                double_quote = (double_quote+1)%2;
                if(single_quote != 0){
                    args[n][m] = ch;
                    m++;
                }
                else if ((line[count_char] == '|' || line[count_char] == '>' || line[count_char] == '<') && line[count_char+1] == '\"'&& (line[count_char+2] == ' ' || line[count_char+2] == '\n'))
                {
                    args[n][m] = line[count_char];
                    args[n][m+1] = 'q';
                    args[n][m+2] = 'u';
                    args[n][m+3] = 'o';
                    //fprintf(stderr, "double quote %s\n", args[n]);
                    m = 0;
                    count_char++;
                    n++;
                }
            }
            else
            {
                args[n][m] = ch;
                m++;
            }
        }
    }// args[] ready
    
    print = 0;
    return n+1;

}

void setArgs(int argc, char** Args, char **args){
    for (int i = 0; i < argc; i++)
    {
        Args[i] = args[i];
    }
    Args[argc] = NULL;
}

