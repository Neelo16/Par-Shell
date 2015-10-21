#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "parshell.h"
#include "commandlinereader.h"


void exitShell(int childCnt) {

    int i;
    int *pidArray = (int*) malloc(sizeof(int)*childCnt); /* saves children pid for exiting the shell */
    int *statusArray = (int*) malloc(sizeof(int)*childCnt); /* same as above for the status */

    if (childCnt != 0 && (pidArray == NULL || statusArray == NULL)) /* checks errors in mallocs (if there are any children) */
        perror("Error allocating memory");

    else {
        for (i = 0; i < childCnt; i++) {
            pidArray[i] = wait(statusArray + i); /* Address of the entry i of status array*/
            if (pidArray[i] == -1)
                perror("Error in wait");
        }
        for (i = 0; i < childCnt; i++)
            if (pidArray[i] != -1 && WIFEXITED(statusArray[i]))
                printf("%d %d\n", pidArray[i], WEXITSTATUS(statusArray[i]));
            else
                printf("%d did not terminate successfully\n", pidArray[i]);
    }
    free(pidArray);
    free(statusArray);
}

void *monitorChildren(void *data){

}


int createProcess(char *argVector[]) {
    /* Returns 1 if sucessful */
    int pid = fork();
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
        int i;
        for(i = 0; i < ARGNUM; i++) 
            argVector[i] = NULL;    
        return 1;           
    }
}

int main(int argc, char const *argv[]) {

	char *argVector[ARGNUM];
	int i; 
    char *user = getenv("USER"); /* Used just to adorn the prompt line (%user%@par-shell) */
    sharedData_t shared = malloc(sizeof(struct sharedData));
    pthread_t monitorThread;

    if (shared == NULL) {
        perror("Error allocating space for shared variables in main");
        return EXIT_FAILURE;
    }

    if (user == NULL)
        user = "user";

	for(i = 0; i < ARGNUM; i++)
		argVector[i] = NULL;

    shared->childCnt = 0;
    pthread_mutex_init(&(shared->mutex), NULL);
    shared->pidList = lst_new();

    pthread_create(&monitorThread, NULL, /* FIXME */, (void*) shared);

    while (1) {
        int numArgs;
        if (isatty(fileno(stdin))) /* checks if input source is a terminal or pipe/file*/
            printf("%s@par-shell$ ", user);
        numArgs = readLineArguments(argVector, ARGNUM);
        if (numArgs < 0)
        {
            fprintf(stderr, "Error reading arguments");
            exit(EXIT_FAILURE);
        }
        else if (numArgs == 0)
            continue;
        if (!strcmp("exit", argVector[0])) {
            exitShell(childCnt);
            return EXIT_SUCCESS;
        }
        else
            if(createProcess(argVector))
                childCnt++;
    }

	
	return EXIT_FAILURE; /* This line should not be executed */
}
