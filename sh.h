
#include "get_path.h"

int pid;
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
void list ( char *dir );
void cd(char **args, int argsct, char **owd, char **pwd, char **homedir); // function to handle built-in cd
void set_prompt(char **args, int argsct, char **prompt); // function to handle built-in prompt
void printenv(char **args, int argsct); // function to handle built-in printenv
void my_setenv(char **args, int argsct, char **homedir, struct pathelement **pathlist, char *command); // function to handle built-int setenv
void exec_process(char *process, char **args, int argsct, char **envp, int *status); // function to handle executing child processes
void exec_command(char *command, int argsct, char *commandpath, char **args, char **owd, char **pwd, char **homedir, struct pathelement *pathlist, int status, char **envp, char **prompt, int *go, int *EC);
void parse_cmd(char *cmdl, char **command, char ***args, char **arg, int *argsct); 
#define PROMPTMAX 32
#define MAXARGS 10
