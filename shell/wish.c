//Add your code here
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#define DELIM " \t\r\n\a"

//The one and only error message
char error_message[30] = "An error has occurred\n";

void print_error(){
    //write(STDERR_FILENO, error_message, strlen(error_message));
	fprintf(stderr,"An error has occurred\n");
}

//Global variable for paths
char *paths[100] = {"/bin"};

int run_cd(char **args,int);
int set_path(char **args,int);
int run_exit(char **args,int);
int perform_command(char line[], int numArgs);
int execute_command(char *new_args[],char *redirect_args[]);
int command_redirect(char* line, char* split_point);
int command_parallel(char* ret, char* line);
int command_direct(char **args, int numArgs);

//To trim whispaces
char *trim(char *s){
    //Works as changing back and front of string's address
    char* back = s + strlen(s);
    while(isspace(*s)) {
        s++;
    }
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
    // last char will be strlen(s)-1
}

//To clear unused tokens
/*char **trim_tokens(char **t){
    int i = 0;
    int j = 0;
    while(*(t+i) != NULL){
        //If string is not blank move its position to first not blank tokens position and trim again in case
        if(strcmp(*(t+i), "") != 0){
            *(t+j) = *(t+i);
            *(t+j) = trim(*(t+j));
            j++;
        }
        else{
            *(t+i) = NULL;
        }
        i++;
    }
    return t;
}*/

char **splitLine(char *line)
{
    char **tokens = (char **)malloc(sizeof(char *) * 64);
    if (!tokens)
    {
        print_error();
        free(tokens);
        exit(EXIT_FAILURE);
    }

    char *token;
    int pos = 0, bufsize = 64;
    token = strtok(line, DELIM);
    while (token != NULL)
    {
        tokens[pos] = token;
        pos ++;
        if (pos >= bufsize)
        {
            bufsize += 64;
            line = realloc(line, bufsize * sizeof(char *));
            if (!line) // Buffer Allocation Failed
            {
            print_error();
            exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, DELIM);
    }
    tokens[pos] = NULL;
    free(token);
    return tokens;
}
int lineSeperate(char* line, char* args[], char* delim) {
    char *save_result;
    int argsIndex = 0;
    //args = malloc(100*sizeof(char*));
    if (!args)
        args = malloc(100*sizeof(char));
    args[argsIndex] = strtok_r(line, delim, &save_result);
    argsIndex++;
    while(1){
        args[argsIndex] = strtok_r(NULL, delim,&save_result);
        if (args[argsIndex] == NULL)
            break;
        argsIndex++;
    }
    if (args[0] == NULL)
        return 0;
    return argsIndex;
}
//To seperate the tokens
/*char **sep(char *s){
    char **token_temp = malloc(sizeof(char*) * 300);
    char **token = malloc(sizeof(char*) * 300);
    char delim[10] = " \t\n\r\a";//" \t\n\r\a";
    int i = 0;
    //Seperate tokens using space delimeter and add them to a temp array
    while(s != NULL){
        *(token_temp+i) = malloc(sizeof(char*) * strlen(s));
        *(token_temp+i) = strsep(&s, " ");
        i++;
    }
    i = 0;
    int k = 0;
    //To clean the tabs from tokens clear and seperate every token again using delimeter \t
    //And add the to real tokens array which I call full
    while(*(token_temp+i) != NULL){
        while(*(token_temp+i) != NULL){
            *(token+k) = malloc(sizeof(char*) * strlen(*(temp+i)));
            *(token+k) = strsep((temp+i), "\t");
            k++;
        }
        i++;
    }
    
    return token;
}*/

int execute_command(char *new_args[],char *redirect_args[])
{
    pid_t pid;
    int status;
    char *full_path = malloc(sizeof(char)*100);
    char *final_path = malloc(sizeof(char)*100);
    if (paths[0] == NULL){
        print_error();
        return 1;
    }
    if (new_args == NULL)
        return 1;
    if (new_args[0] == NULL)
        return 1;

    //printf("copy args\n");
    int i = 0;
    while(paths[i] != NULL){
        strcat(strcat(strcpy(full_path, paths[i]),"/"), new_args[0]);
        if(access(full_path, X_OK) == 0){
            strcpy(final_path, full_path);
            break;
        }
        i++;
    }
    //Fork - Wait - Exec functions
    pid = fork();
    if (pid == 0) {
        if(redirect_args){
            //Used 0600 for permissions and others to only create and write to file
            //printf("execute %s\n",redirect_args[0]);
            int fd_out = open(redirect_args[0],O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);// O_WRONLY | O_CREAT, 0600);
            dup2(fd_out, 1);   //make stdout go to file
            dup2(fd_out, 2);   //make stderr go to file
            close(fd_out); 
        }
        if (execv(final_path, new_args) == -1) {
            print_error();
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        //Error forking
        print_error();
    } else {
        //Parent process
        do {
        waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    free(full_path);
    free(final_path);
    return 0;
}
int command_redirect(char* line, char* split_point)
{
    char* Args_command[100];
    char* Args_parameter[100];
    split_point[0] = '\0';
    split_point = split_point + 1;
    int argsNum = lineSeperate(line, Args_command,DELIM);
    //printf("%s\n",Args_command[0]);
    //printf("%d\n",argsNum);
    //printf("num=%d\n",argsNum);
    //printf("param=%s\n",line);
    if (argsNum == 0){
        print_error();
        return 1;
    }
    int restArgc = lineSeperate(split_point, Args_parameter,DELIM);
    //printf("%s\n",Args_parameter[0]);
    //printf("%s\n",Args_parameter[1]);
    //printf("%d\n",restArgc);
    if (restArgc != 1){
        print_error();
        return 1;
    }
    return execute_command(Args_command, Args_parameter);
}
int command_parallel(char* ret, char* line){
    char** commands_array = malloc(100*sizeof(char*));
    int commands_total = lineSeperate(line, commands_array,"&");
    char** paramater_each = malloc(50*sizeof(char*));
    //char* split_point = malloc(100*sizeof(char));
    for (int i = 0; i < commands_total; i++) {
        /*if ((split_point = strchr(commands_array[i], '>'))){
            //printf("redirect %s\n",commands_array[i]);
            command_redirect( commands_array[i],split_point);
            continue;
        }
        //printf("%s\n",commands_array[i]);
        lineSeperate(commands_array[i], paramater_each, DELIM);
        execute_command(paramater_each, NULL);*/
        perform_command(commands_array[i],1);
    }
    free(paramater_each);
    free(commands_array);
    return 0;
}
char *command_keyword[] = {"cd", "path","exit"};
int (*command_func[]) (char **,int) = {&run_cd,&set_path, &run_exit}; // Array of function pointers for call from execShell

int set_path(char **args, int numArgs)
{

	if(args[1] == NULL){
        int j = 0;
        while(paths[j] != NULL){
            //paths[j] = NULL;
            free(paths[j]);
            j++;
        }
        paths[0]="";
    }
    int i = 0;
    while(args[i+1] != NULL){
        //printf("%ld\n",strlen(args[i + 1]));
        paths[i] = malloc(strlen(args[i+1]) + 1);// malloc(sizeof(char*) * strlen(args[i + 1]));
        strcpy(paths[i], args[i + 1]);
        //printf("%s;", paths[i]);
        i++;
    }
    return 0;
}
int run_cd(char **args, int numArgs)
{
    if(numArgs != 2){
            print_error();
            return 1;
    }else{
        if(chdir(args[1]) !=0)
		{
			print_error();
            return 1;
		}
    }
    return 0;
}
int run_exit(char **args, int numArgs)
{
	if(args[1] == NULL)
		exit(0);
	else
		print_error();
        return 1;
    return 0;
}
int numCommand() // Function to return number of builtin commands
{
    return sizeof(command_keyword)/sizeof(char *);
}

//This checks and runs the args
int command_direct(char **args, int numArgs){
    //Empty input
    if(args[0] == NULL){
        return 1;
    }
	//printf("%s\n",args[0]);
	//printf("%s\n",args[1]);
    //Exit builtin
	for (int i=0; i< numCommand(); i++) 
    {
        if(strcmp(args[0], command_keyword[i])==0) 
            return (*command_func[i])(args,numArgs); 
    }
    if(paths[0] == NULL){
        print_error();
    }
    else{
        execute_command(args,NULL);
    }
	return 0;
}
int perform_command(char line[], int numArgs)
{
    char *split_point=NULL;
    char **tokens;
    int result=0;
    if ((split_point=strchr(line, '&'))){
                
        return command_parallel(split_point, line);
    }
    else if ((split_point=strchr(line,'>')))
    {
        //printf("%s",split_point);split_point='> output.9 output.10....';
        return command_redirect(line,split_point);
    }
    else{
        char *input;
        input=line;
        tokens = splitLine(input);
		result= command_direct(tokens,numArgs);
        free(tokens);
    }
    free(split_point);
    return result;
}
int readfile(char filename[100], int numArgs)
{
    FILE *fptr;
    char line[200];
    fptr = fopen(filename, "r");
	//
	int result=1;
    //size_t size = 0;
    if (fptr == NULL)
    {
		print_error();
        exit(EXIT_FAILURE);
        return 1;
    }
    else
    {
        while(fgets(line, sizeof(line), fptr)!= NULL)
        {
            if ((strcmp(line, "\n") == 0) || (strcmp(line, "") == 0))
                continue;
            //omit the last \n of the string
            if (line[strlen(line) - 1] == '\n')
                line[strlen(line) - 1] = '\0';
            //exit if EOF is read
            if (line[0] == EOF){
                return 1;
            }
			//printf("%s\n",line);
			//input = trim(line);
			////trim_tokens(
			/*printf("=input=%s\n",input);
			printf("=0=%s\n",tokens[0]);
			printf("=1=%s\n",tokens[1]);
			printf("=2=%s\n",tokens[2]);
			printf("=3=%s\n",tokens[3]);*/
            result=perform_command(line,numArgs);
        }
		fseek (fptr, 0, SEEK_END);
		int size = ftell(fptr);
		if (0 == size) {
			print_error();
			return 0;
		}
            
    }
    fclose(fptr);
    //free(input);
    //free(tokens);
    return result;
}

int main(int argc, char *argv[]) {
	//printf("%d",argc);
	if (argc == 2)
	{
        exit(readfile(argv[1],argc));
	}
    else{
        //printf("\nInvalid Number of Arguments");
		print_error();
		exit(1);
	}
	exit(0);
}