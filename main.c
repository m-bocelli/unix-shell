#include "sh.h"
#include <signal.h>
#include <stdio.h>

void sig_handler(int signal);

int main( int argc, char **argv, char **envp )
{
	// IGNORE THE FOLLOWING SIGNALS
	struct sigaction sa = { 0 };

	sa.sa_handler = &sig_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
		/* if signal is ctrl-c */
		if (sigaction(SIGINT, &sa, NULL) == -1 ) {
			perror("sigaction");
			exit(EXIT_FAILURE);
		} 

		/* if signal is ctrl-z */
		if (sigaction(SIGTSTP, &sa, NULL) == -1) {
			perror("sigaction");
			exit(EXIT_FAILURE);
		}

		/* if signal is SIGTERM */
		if (sigaction(SIGTERM, &sa, NULL) == -1) {
			perror("sigaction");
			exit(EXIT_FAILURE);
		}

	return sh(argc, argv, envp);
}

void sig_handler(int signal)
{
	// Do nothing upon receiving signal
}

