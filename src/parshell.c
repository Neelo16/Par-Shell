#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "parshell.h"
#include "commandlinereader.h"

#define ARGNUM 7						

int main(int argc, char const *argv[])
{
	char *argvector[ARGNUM];
	int i;
	int childCnt = 0;

	for(i=0;i<ARGNUM;i++)
		argvector[i] = NULL;

    while (1) {
        int numArgs = readLineArguments(argvector, ARGNUM);
        if (numArgs < 0)
        {
            perror("Error reading arguments");
            exit(EXIT_FAILURE);
        }
        if (!strcmp("exit", argvector[0])) {
            int *pidArray = (int*) malloc(sizeof(int)*childCnt);
            int *statusArray = (int*) malloc(sizeof(int)*childCnt);
            if (childCnt != 0 && (pidArray == NULL || statusArray == NULL))
                perror("Error allocating memory");

            else {
                for (i = 0; i < childCnt; i++) {
                    pidArray[i] = wait(statusArray + i); /* Address of the entry i of status array*/
                    if (!WIFEXITED(statusArray[i]))
                        perror("Error ocurred in child process");
                }
                for (i = 0; i < childCnt; i++)
                    printf("%d %d", pidArray[i], statusArray[i]);
            }
            free(pidArray);
            free(statusArray);
        }
        else {
            int pid = fork();
            if (pid < 0) {
                perror("Error forking process");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0) {
                execv(argvector[0], argvector);
                perror("Error executing process");
                exit(EXIT_FAILURE);
            }
            else
                childCnt++;

        }
    }

	
	return EXIT_SUCCESS;
}