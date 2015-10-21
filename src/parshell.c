#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "parshell.h"
#include "commandlinereader.h"


void exitShell(int childCnt) {


       
       /* for (i = 0; i < childCnt; i++)
            if (pidArray[i] != -1 && WIFEXITED(statusArray[i]))
                printf("%d %d\n", pidArray[i], WEXITSTATUS(statusArray[i]));
            else
                printf("%d did not terminate successfully\n", pidArray[i]);*/
}

void *monitorChildren(void *data){
	sharedData_t data = (sharedData_t) data;
	time_t endtime;
	int status;
	int pid;
	while(1) {
		sem_wait(&(data->sem));				
		pthread_mutex_lock(&data->mutex);
		if(data->childCnt == 0 && data->exited){
			pthread_mutex_unlock(&data->mutex);
			pthread_exit(NULL);
		}
		pthread_mutex_unlock(&data->mutex);
		pid = wait(&status);
		if (pid == -1)
                perror("Error in wait");
		endtime = time(NULL);
		if(endtime == (time_t) -1) 
			fprintf(stderr, "Error on getting child endtime\n");

		pthread_mutex_lock(&data->mutex);
		update_terminated_process(data->pidList, pid, endtime, status);
		data->childCnt--;
		pthread_mutex_unlock(&data->mutex);
	}
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
	shared->exited = 0;
    pthread_mutex_init(&(shared->mutex), NULL);
	sem_init(&shared->sem,0,0);
    shared->pidList = lst_new();

    pthread_create(&monitorThread, NULL, monitorChildren, (void*) shared);

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
