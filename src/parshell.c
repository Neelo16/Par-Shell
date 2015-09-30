#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "parshell.h"
#include "commandlinereader.h"

#define ARGNUM 7						// nome_comando + 5 argumentos(maximo) + argumento NULL

int main(int argc, char const *argv[])
{
	char *argvector[ARGNUM];
	int i;
	int childCnt = 0;

	// Inicializar o vetor argvector
	for(i=0;i<ARGNUM;i++)
		argvector[i] = NULL;

    while (1) {
        int numArgs = readLineArguments(argvector, ARGNUM);
        if (numArgs < 0)
        {
            perror("Error reading arguments");
            exit(1);
        }
        if (!strcmp("exit", argvector[0])) {
            int *pidArray = (int*) malloc(sizeof(int)*childCnt);
            int *statusArray = (int*) malloc(sizeof(int)*childCnt);
            for (i = 0; i < childCnt; i++) {
                pidArray[i] = wait(statusArray + i); /* Address of the entry i of status array*/
                
            }
        }
    }

	//exit
	
	return 0;
}