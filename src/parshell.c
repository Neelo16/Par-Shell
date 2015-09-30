#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "parshell.h"
#include "commandlinereader.h"


void exitShell(int childCnt) {

    int *pidArray = (int*) malloc(sizeof(int)*childCnt); /* saves children pid for exiting the shell */
    int *statusArray = (int*) malloc(sizeof(int)*childCnt); /* same as above for the status */

    if (childCnt != 0 && (pidArray == NULL || statusArray == NULL)) /* checks errors in mallocs (there has to be children) */
        perror("Error allocating memory");

    else {
        for (i = 0; i < childCnt; i++) {
            pidArray[i] = wait(statusArray + i); /* Address of the entry i of status array*/
            if (!WIFEXITED(statusArray[i])) /* checks if child process terminated properly */
                perror("Error ocurred in child process");
        }
        for (i = 0; i < childCnt; i++)
            printf("%d %d\n", pidArray[i], WEXITSTATUS(statusArray[i]));
    }
    free(pidArray);
    free(statusArray);
}

int createProcess(char *argVector) {
    int pid = fork();
    int i;
    if (pid < 0) {
        perror("Error forking process");
        return 0;
    }
    else if (pid == 0) {
        execv(argVector[0], argVector);
        perror("Error executing process");
        exit(EXIT_FAILURE);
    }
    else {
        free(*argVector);
        for(i = 0; i < ARGNUM; i++) 
            argVector[i] = NULL;    
        return 1;           
    }
}

int main(int argc, char const *argv[]) {

	char *argVector[ARGNUM];
	int i; 
	int childCnt = 0;

	for(i=0;i<ARGNUM;i++)
		argVector[i] = NULL;

    while (1) {
        int numArgs = readLineArguments(argVector, ARGNUM);
        if (numArgs < 0)
        {
            perror("Error reading arguments");
            exit(EXIT_FAILURE);
        }
        if (!strcmp("exit", argVector[0])) {
            exitShell(childCnt);
            free(*argVector);
            break;
        }
        else
            childCnt += createProcess(argVector);
    }

	
	return EXIT_SUCCESS;
}