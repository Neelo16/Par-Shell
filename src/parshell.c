#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "parshell.h"
#include "commandlinereader.h"


void *monitorChildren(void *arg){
	sharedData_t data = (sharedData_t) arg;
	time_t endtime;
	int status;
	int pid;
	while(1) {
		sem_wait(&data->sem);				
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


int createProcess(char *argVector[], list_t *pidList) {
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
        time_t starttime = time(NULL);
        for(i = 0; i < ARGNUM; i++) 
            argVector[i] = NULL;    
        insert_new_process(pidList, pid, starttime);
        return 1;           
    }
}


void exitShell(sharedData_t data,pthread_t monitorThread){
    pthread_mutex_lock(&data->mutex);
    data->exited = 1;
    pthread_mutex_unlock(&data->mutex);

    sem_post(&data->sem); /* Unlocks monitor thread in order to complete exit procedures */

    if (pthread_join(monitorThread, NULL))
        fprintf(stderr, "Error waiting for monitoring thread");
    lst_print(data->pidList);

	if (pthread_mutex_destroy(&data->mutex))
		fprintf(stderr, "Error destroying mutex\n");

	if (sem_destroy(&data->sem))
		perror("Error destroying semaphore");

	lst_destroy(data->pidList);
	free(data);
}


int main(int argc, char const *argv[]) {

	int i;
	char buffer[BUFFER_SIZE];
	char *argVector[ARGNUM]; 
    char *user = getenv("USER"); /* Used just to adorn the prompt line (%user%@par-shell) */
    sharedData_t data = malloc(sizeof(struct sharedData));
    pthread_t monitorThread;

    if (data == NULL) {
        perror("Error allocating space for shared variables in main");
        return EXIT_FAILURE;
    }

    if (user == NULL)
        user = "user";

	for(i = 0; i < ARGNUM; i++)
		argVector[i] = NULL;

    data->childCnt = 0;
	data->exited = 0;
    pthread_mutex_init(&data->mutex, NULL);
	sem_init(&data->sem,0,0);
    data->pidList = lst_new();

    pthread_create(&monitorThread, NULL, monitorChildren, (void*) data);

    while (1) {
        int numArgs;
        if (isatty(fileno(stdin))) /* Checks if input source is a terminal or pipe/file */
            printf("%s@par-shell$ ", user);
        numArgs = readLineArguments(argVector, ARGNUM, buffer, BUFFER_SIZE);
        if (numArgs < 0)
        {
            fprintf(stderr, "Error reading arguments");
            exitShell(data,monitorThread);
            return EXIT_FAILURE;
        }
        else if (numArgs == 0)
            continue;
        if (!strcmp("exit", argVector[0])) {
            exitShell(data,monitorThread);
            return EXIT_SUCCESS;
        }
        else {
            pthread_mutex_lock(&data->mutex);
            if(createProcess(argVector, data->pidList)) {
                data->childCnt++;
                sem_post(&data->sem);
            }
            pthread_mutex_unlock(&data->mutex);
        }
    }

	
	return EXIT_FAILURE; /* This line should not be executed */
}
