#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <glob.h>
#include "sh.h"

extern char** environ;
extern int errno;

int sh( int argc, char **argv, char **envp )
{
	char *prompt = calloc(PROMPTMAX, sizeof(char));
	char *commandline = calloc(MAX_CANON, sizeof(char));
	char *command, *arg, *commandpath, *pwd, *owd;
	char **args = calloc(MAXARGS, sizeof(char*));
	int uid, status, argsct, go = 1;
	struct passwd *password_entry;
	char *homedir;
	struct pathelement *pathlist;
	int EC = 0;

	uid = getuid();
	password_entry = getpwuid(uid);               /* get passwd info */
	homedir = password_entry->pw_dir;		/* Home directory to start
							   out with*/

	if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
	{
		perror("getcwd");
		exit(2);
	}
	owd = calloc(strlen(pwd) + 1, sizeof(char));
	memcpy(owd, pwd, strlen(pwd));
	prompt[0] = ' '; prompt[1] = '\0';

	/* Put PATH into a linked list */
	pathlist = get_path();

	/* Put path to curr prog in env var 0 */
	setenv("0", argv[0], 1);

	/* Initialize ACC env var */
	if (getenv("ACC") == NULL) {
		setenv("ACC", "0", 1);
	}

	while ( go )
	{
		argsct = 0;
		if (argc == 1) {
			/* print your prompt */
			printf("%s [%s]> ", prompt, pwd); 
			/* get command line and process */
			if (fgets(commandline, MAX_CANON, stdin) != NULL) {
				if (commandline[strlen(commandline)-1]  == '\n') {
					commandline[strlen(commandline)-1] = 0;
				}

				/* if command starts with ?, check condition of env */
				if(commandline[0] == '?') {
					if(getenv("?") != NULL && strcmp(getenv("?"), "0") != 0) {
						continue;
					} else {
						char new_cmdl[strlen(commandline)];
						memmove(new_cmdl, &commandline[1], strlen(commandline)-1);
						new_cmdl[strlen(commandline)-1] = '\0';
						strcpy(commandline,new_cmdl);
					}
				}

				/* parse command (first 'word' on cmdline) */
				command = strtok(commandline, " ");
				if (command == NULL) {
					continue;
				}

				/* if command starts with $, substitute with env if possible */
				if (command[0] == '$') {
					char sub_cmd[strlen(command)];
					memmove(sub_cmd, &command[1], strlen(command)-1);
					sub_cmd[strlen(command)-1] = '\0';	
					char *env_val = getenv(sub_cmd);
					if (env_val == NULL) {
						command = "";
					} else {
						command = env_val;
					}

				}

				arg = strtok(NULL, " ");

				int i = 0;
				/* parse all args in the command line */
				while (arg != NULL && i < MAXARGS) {
					/* substitute each arg that starts with $ if possible */
					if (arg[0] == '$') {
						char sub_arg[strlen(arg)];
						memmove(sub_arg, &arg[1], strlen(arg)-1);
						sub_arg[strlen(arg)-1] = '\0';	
						char *env_val = getenv(sub_arg);
						if (env_val == NULL) {
							arg = "";
						} else {
							arg = env_val;
						}
					}
					args[i++] = arg;
					arg = strtok(NULL, " ");	
					argsct++;
				}
			} else {
				// EOF or illegal character detected
				clearerr(stdin);
				puts("");
				continue;
			}	

			exec_command(command, argsct, commandpath, args, &owd, &pwd, &homedir, pathlist, status, envp, &prompt, &go, &EC);

			/* ARGUMENT PASSED TO EXECUTABLE */
		} else {
			FILE *file = fopen( argv[1], "r" ); 
			char *cmdl = NULL;
			size_t len = 0;
			if ( file == 0 ) 
			{ 
				printf( "Could not open file\n" ); 
			} 
			else  
			{ 
				while  ( len = getline(&cmdl, &len, file)  != -1 && go) 
				{ 
					argsct = 0;
					cmdl[strlen(cmdl)-1] = 0;

					if(cmdl[0] == '?') {
						if(getenv("?") != NULL && strcmp(getenv("?"), "0") != 0) {
							continue;
						} else {
							char new_cmdl[strlen(cmdl)];
							memmove(new_cmdl, &cmdl[1], strlen(cmdl)-1);
							new_cmdl[strlen(cmdl)-1] = '\0';
							strcpy(cmdl,new_cmdl);
						}
					}

					/* parse command (first 'word' on cmdline) */
					command = strtok(cmdl, " ");
					if (command == NULL) {
						continue;
					}

					if (command[0] == '$') {
						char sub_cmd[strlen(command)];
						memmove(sub_cmd, &command[1], strlen(command)-1);
						sub_cmd[strlen(command)-1] = '\0';	
						char *env_val = getenv(sub_cmd);
						if (env_val == NULL) {
							command = "";
						} else {
							command = env_val;
						}

					}

					arg = strtok(NULL, " ");

					int i = 0;
					/* parse all args in the command line */
					while (arg != NULL && i < MAXARGS) {
						if (arg[0] == '$') {
							char sub_arg[strlen(arg)];
							memmove(sub_arg, &arg[1], strlen(arg)-1);
							sub_arg[strlen(arg)-1] = '\0';	
							char *env_val = getenv(sub_arg);
							if (env_val == NULL) {
								arg = "";
							} else {
								arg = env_val;
							}
						}
						args[i++] = arg;
						arg = strtok(NULL, " ");	
						argsct++;
					}

					exec_command(command, argsct, commandpath, args, &owd, &pwd, &homedir, pathlist, status, envp, &prompt, &go, &EC);

				}	 
				free(cmdl);
				fclose( file ); 
				go = 0;
				EC = atoi(getenv("?"));
			}
		}
	}
	/* freeing allocated memory */
	free(commandline);
	free(prompt);
	free(owd);
	free(pwd);
	free_path(pathlist);
	free(args);
	exit(EC);
} /* sh() */

/* Conditional checks which command to run, and clears args */
void exec_command(char *command, int argsct, char *commandpath, char **args, char **owd, char **pwd, char **homedir, struct pathelement *pathlist, int status, char **envp, char **prompt, int *go, int *EC) 
{
	/* check for each built in command and implement */
	// WHICH
	if (strcmp(command, "which") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		for(int i = 0; i < argsct; i++) {
			commandpath = which(args[i], pathlist);
			if (commandpath != NULL) {
				printf("%s\n", commandpath);
				free(commandpath);
			} else {
				printf("%s not found\n", args[i]);
			}
		}
		// CD
	} else if (strcmp(command, "cd") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		cd(args, argsct, owd, pwd, homedir);
		// PWD
	} else if (strcmp(command, "pwd") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		printf("%s\n", *pwd);
		// LIST
	} else if (strcmp(command, "list") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		if(argsct == 0) {
			list(*pwd);
		} else {
			for(int i = 0; i < argsct; i++) {
				puts("");
				list(args[i]);
			}
		}	
		// PID
	} else if (strcmp(command, "pid") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		printf("%d\n",(int)getpid());
		// PROMPT
	} else if (strcmp(command, "prompt") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		set_prompt(args, argsct, prompt);
		// PRINTENV
	} else if (strcmp(command, "printenv") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		printenv(args, argsct);
		// SETENV
	} else if (strcmp(command, "setenv") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		my_setenv(args, argsct, homedir, &pathlist, command);
		// ADDACC
	} else if (strcmp(command, "addacc") == 0) {
		int new_acc = atoi(getenv("ACC"));
		if (argsct) {
			/* if there's an arg, add it */
			new_acc += atoi(args[0]);
		} else {
			/* inc by one */
			new_acc++;
		}
		char new_acc_str[10];
		sprintf(new_acc_str, "%d", new_acc);
		setenv("ACC", new_acc_str, 1);
		// EXIT
	} else if (strcmp(command, "exit") == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing built-in %s\n", command);
		if (argsct)
			*EC = atoi(args[0]);
		printf("Exit code: %d\n", *EC);	
		char exit_str[10];
		sprintf(exit_str, "%d", *EC);
		setenv("?", exit_str, 1);
		*go = 0;
		// EXTERNAL PROGRAM
	} else if (command[0] == '/' || command[0] == '.') {
		if (access(command, X_OK) == 0) {
			// execute if accessible as an executable
			exec_process(command, args, argsct, envp, &status);
		} else {
			perror("access");
		}
		// PROGRAM ON PATH?
	} else {
		/* find it */
		commandpath = which(command,pathlist);
		if (commandpath != NULL) {
			// execute if command can be found
			exec_process(commandpath, args, argsct, envp, &status);
			free(commandpath);
		} else {
			fprintf(stderr, "%s: Command not found.\n", command);
		}
	}
	/* clear args */
	for(int i = 0; i < argsct; i++) {
		args[i] = NULL;
	}
}

/* Implements built-in which command */
char *which(char *command, struct pathelement *pathlist )
{
	/* loop through pathlist until finding command and return it.  Return
	   NULL when not found. */
	struct pathelement *p = pathlist;

	char* buff = calloc(PATH_MAX + strlen(command), sizeof(char));
	while (p != NULL) {
		sprintf(buff, "%s/%s", p->element, command);
		if (access(buff, X_OK) == 0) {
			return buff;
		}
		p = p->next;
	}
	free(buff);
	return NULL;

} /* which() */

/* Implements built-in list command */
void list ( char *dir )
{
	// initialize glob buffer
	glob_t globbuff;
	globbuff.gl_pathc = 0;
	glob(dir, GLOB_NOCHECK, NULL, &globbuff);
	for (int i = 0;  i < globbuff.gl_pathc; i++) {
		// open directory stream
		DIR* ds = opendir(globbuff.gl_pathv[i]);
		struct dirent *file;
		// if directory
		if (ds != NULL) {
			printf("%s:\n", globbuff.gl_pathv[i]);
			while (file = readdir(ds)) {
				// print contents
				puts(file->d_name);
			}
			closedir(ds);
			puts("");
		} else {
			perror("list");
		} 
	}
	if (globbuff.gl_pathc > 0)
		globfree(&globbuff);
} /* list() */

/* Implements built-in chdir() as cd command */
void cd(char** args, int argsct, char** owd, char** pwd, char** homedir) {
	// When no args are provided, go to homdir
	if (argsct == 0) {
		free(*owd);
		*owd = calloc(strlen(*pwd) + 1, sizeof(char));
		memcpy(*owd, *pwd, strlen(*pwd));
		if (chdir(*homedir) == -1) {
			perror("chdir");
			exit(2);
		} else {
			free(*pwd);
			if ( (*pwd = getcwd(NULL, PATH_MAX+1)) == NULL ) {
				// getcwd fails
				perror("getcwd");
				exit(2);
			}

		}
		// Do nothing when too many args are provided
	} else if (argsct > 1) {
		fprintf(stderr, "cd: too many args\n");
		// Change to prev dir
	} else if (strcmp(args[0], "-") == 0) {
		// save old working directory
		char *tmp_owd = calloc(strlen(*pwd) + 1, sizeof(char));
		// make current dir owd
		memcpy(tmp_owd, *pwd, strlen(*pwd));
		// go to owd
		chdir(*owd);
		free(*pwd);
		if ( (*pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
		{
			perror("getcwd");
			exit(2);
		}
		// set previous pwd as new owd
		free(*owd);
		*owd = calloc(strlen(tmp_owd) + 1, sizeof(char));
		memcpy(*owd, tmp_owd, strlen(tmp_owd));
		free(tmp_owd);
		// Change dir
	} else { 
		if (chdir(args[0]) == -1) {
			perror("cd");
		} else {
			free(*owd);
			*owd = calloc(strlen(*pwd) + 1, sizeof(char));
			memcpy(*owd, *pwd, strlen(*pwd));
			free(*pwd);
			if ( (*pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
			{
				perror("getcwd");
				exit(2);
			}	
		}
	}
} /* cd() */

/* Implementation of built-in prompt command */
void set_prompt(char **args, int argsct, char **prompt) {
	if (argsct == 0) {
		printf("input prompt prefix: ");
		char buff[PROMPTMAX];
		// get prompt from stdin buffer and replace current prompt
		if (fgets(buff, PROMPTMAX, stdin) != NULL) {
			buff[strlen(buff)-1] = '\0';
			strcpy(*prompt,buff);
		}
	} else {
		// use first arg as prompt
		strcpy(*prompt, args[0]);
	}
}

/* Implementation of built-in prinenv command */
void printenv(char **args, int argsct) {
	if (argsct == 0) {
		// print every environment variable
		for(int i = 0; environ[i] != NULL; i++) {
			printf("%s\n", environ[i]);
		}	
	} else {
		// print all environment variables provided as args
		for(int i = 0; i < argsct; i++) {
			char* env = getenv(args[i]);
			if (env == NULL) {
				fprintf(stderr, "%s is not in this environment\n", args[i]);
			} else {
				printf("%s=%s\n", args[i], env);
			}
		}
	}
}

/* Implementation of built-in setenv command */
void my_setenv(char **args, int argsct, char **homedir, struct pathelement **pathlist, char *command) {
	if (argsct == 0) {
		// print every environment variable
		printenv(args, argsct);
	} else if (argsct == 1) {
		// set environment variable to null string when no value provided
		int set_res = setenv(args[0], "", 1);
		if (set_res == -1) {
			perror("setenv");
		}
	} else if (argsct == 2) {
		// set environment variable to valu provided
		int set_res = setenv(args[0], args[1], 1);
		if (set_res == -1) {
			perror("setenv");
		} else if (strcmp(args[0], "PATH") == 0) {
			// reconstruct PATH env if altered
			free_path(*pathlist);
			*pathlist = get_path();
		} else if (strcmp(args[0], "HOME") == 0) {
			// replace homedir if altered
			strcpy(*homedir, args[1]);
		}
	} else {
		fprintf(stderr, "%s: Too many arguments\n", command);
	}
}

/* Implementation for executing a non-built-in command */
void exec_process(char *process, char** args, int argsct, char **envp, int *status) {
	// initialize glob buffer
	glob_t globbuf;
	globbuf.gl_pathc = 0;
	int globbed = 0;

	// expand all args that can be globbed, GLOB_NOCHECK keeps original if cannot
	for (int i = 0; i < argsct; i++) {
		if (!globbed) {
			glob(args[i], GLOB_NOCHECK, NULL, &globbuf);
			globbed = 1;
		} else {
			// append glob results for repeated calls
			glob(args[i], GLOB_NOCHECK | GLOB_APPEND, NULL, &globbuf);
		}
	}

	// create args array that conforms to execve standard of command in args[0]
	char **args_ex = calloc(globbuf.gl_pathc + 2, sizeof(char *));
	// null terminated
	args_ex[globbuf.gl_pathc + 1] = NULL;
	// first arg is commmand/process
	args_ex[0] = process;


	// fill in globbed args
	for (int i = 0; i < globbuf.gl_pathc; i++) {
		args_ex[i+1] = globbuf.gl_pathv[i];
	}

	// fork new process
	pid = (int)fork();
	if (pid == -1) {
		fprintf(stderr, "Failed to fork\n");
	} else if (pid == 0) {
		if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0)
			printf("Executing %s\n", process);
		// execute process, replacing fork clone
		envp = environ;
		if(execve(args_ex[0], args_ex, envp) == -1) {
			perror("execve");
			free(args_ex);
			exit(2);
		}
	} else {
		// wait for child to finish
		waitpid(pid, status, 0);
		if (WIFEXITED(*status)) {
			// get exit status of nonzero
			int exit_status = WEXITSTATUS(*status);
			char exit_str[10];
			sprintf(exit_str, "%d", exit_status);
			setenv("?", exit_str, 1);
			if (exit_status != 0)
				printf("Child process exited with status: %d\n", exit_status);
		} else { 
			fprintf(stderr, "Child process did not exit successfully, status: %d\n", *status);
		}
	}
	// free glob buffer and args array
	if (globbuf.gl_pathc > 0) 
		globfree(&globbuf);
	free(args_ex);
}
