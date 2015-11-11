#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include "util.h"
#include "list.h"
#include "parshell.h"
#include "commandlinereader.h"

void *monitorChildren(void *arg) {
    sharedData_t data = (sharedData_t) arg;
    time_t endtime;
    int executionTime;
    int status;
    int pid;
    while(1) {             
        mutexLock(&data->mutex);
		while(data->childCnt == 0 && !data->exited)
			condWait(&data->childCntCond,&data->mutex);
        if(data->exited && data->childCnt == 0) {
            mutexUnlock(&data->mutex);
            pthread_exit(NULL);
        }
        mutexUnlock(&data->mutex);
        pid = wait(&status);
        if (pid == -1)
            perror("Error in wait");
        endtime = time(NULL);
        if(endtime == (time_t) -1) 
            fprintf(stderr, "Error getting child endtime\n");

        mutexLock(&data->mutex);
        update_terminated_process(data->pidList, pid, endtime, status);
        data->childCnt--;
        if (pid != -1) {
            executionTime = get_execution_time(data->pidList, pid);
            data->currentIteration++;
            fprintf(data->logFile, "iteracao %d\npid: %d ", data->currentIteration, pid);
            if(executionTime != -1) {
            	data->totalRuntime += executionTime;
                fprintf(data->logFile, "execution time: %d s\n"
                                       "total execution time: %d s\n", 
                                       executionTime, data->totalRuntime);
            }
            else 
            	fprintf(data->logFile, "execution time: Undetermined\n"
                                       "total execution time: %d s\n", 
                                       data->totalRuntime);
         	if (fflush(data->logFile)) perror("Error flushing to file");
        }
        mutexUnlock(&data->mutex);
		condSignal(&data->procLimiterCond);
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
        time_t starttime = time(NULL);
        if (!insert_new_process(pidList, pid, starttime))
            fprintf(stderr, "Failed to save info for process %d"
                            ", will not display process info on exit\n", pid);
        return 1;
    }
}


void exitShell(sharedData_t data,pthread_t monitorThread) {
    mutexLock(&data->mutex);
    data->exited = 1;
    mutexUnlock(&data->mutex);
	condSignal(&data->childCntCond);

    if (pthread_join(monitorThread, NULL))
        fprintf(stderr, "Error waiting for monitoring thread.\n");

    fclose(data->logFile); /* ERROR CHEKC? */

    lst_print(data->pidList);

    if (pthread_mutex_destroy(&data->mutex))
        fprintf(stderr, "Error destroying mutex.\n");

	pthread_cond_destroy(&data->childCntCond);
	pthread_cond_destroy(&data->procLimiterCond);

    lst_destroy(data->pidList);
    free(data);
}

int main(int argc, char const *argv[]) {

    int i;
    char buffer[BUFFER_SIZE];
    char *argVector[ARGNUM]; 
    sharedData_t data = (sharedData_t) malloc(sizeof(struct sharedData));
    pthread_t monitorThread;

    if (data == NULL) {
        perror("Error allocating space for shared variables in main");
        return EXIT_FAILURE;
    }

    data->logFile = fopen("log.txt", "a+");
    if (data->logFile == NULL) {
        perror("Failed to open log file");
        return EXIT_FAILURE;
    }
    
    data->currentIteration = (getNumLines(data->logFile) / 3) - 1; /* comment for dummies needed */
	data->totalRuntime = getTotalRuntime(data->logFile);

    data->pidList = lst_new();
    if (data->pidList == NULL) {
        fprintf(stderr, "Failed to create list to save processes.\n");
        return EXIT_FAILURE;
    }

    data->childCnt = 0;
    data->exited = 0; 
    /* Exited issues the exit command to the monitor thread (ie. 1 means par-shell wants to exit) */

    pthread_mutex_init(&data->mutex, NULL);
	pthread_cond_init(&data->childCntCond,NULL);
	pthread_cond_init(&data->procLimiterCond,NULL);

    if (pthread_create(&monitorThread, NULL, monitorChildren, (void*) data)) {
        fprintf(stderr, "Failed to create thread.\n");
        return EXIT_FAILURE;
    }

    for(i = 0; i < ARGNUM; i++)
        argVector[i] = NULL;

    while (1) {
        int numArgs;
        numArgs = readLineArguments(argVector, ARGNUM, buffer, BUFFER_SIZE);
        if (numArgs < 0) {
            fprintf(stderr, "Error reading arguments\n");
            exitShell(data, monitorThread);
            return EXIT_FAILURE;
        }
        else if (numArgs == 0)
            continue;
        if (!strcmp("exit", argVector[0])) {
            exitShell(data, monitorThread);
            return EXIT_SUCCESS;
        }
        else {
            mutexLock(&data->mutex);
			while(data->childCnt == MAXPAR)
				condWait(&data->procLimiterCond,&data->mutex);
            if(createProcess(argVector, data->pidList)) {
                data->childCnt++;
				condSignal(&data->childCntCond);
            }
            mutexUnlock(&data->mutex);
        }
    }
    return EXIT_FAILURE; /* This line should not be executed */
}
