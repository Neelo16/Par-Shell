#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include "list.h"
#include "parshell.h"
#include "commandlinereader.h"

void *monitorChildren(void *arg) {
    sharedData_t data = (sharedData_t) arg;
    time_t endtime;
    int status;
    int pid;
    while(1) {             
        mutexLock(&data->mutex);
		while(data->childCnt == 0 && data->exited == 0)
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

    lst_print(data->pidList);

    if (pthread_mutex_destroy(&data->mutex))
        fprintf(stderr, "Error destroying mutex.\n");

	pthread_cond_destroy(&data->childCntCond);
	pthread_cond_destroy(&data->procLimiterCond);

    lst_destroy(data->pidList);
    free(data);
}


void mutexLock(pthread_mutex_t *mutex) {
    if (pthread_mutex_lock(mutex)) {
        fprintf(stderr, "Error locking the mutex.\n");
        exit(EXIT_FAILURE);
    }
}


void mutexUnlock(pthread_mutex_t *mutex) {
    if (pthread_mutex_unlock(mutex)) {
        fprintf(stderr, "Error unlocking the mutex.\n");
        exit(EXIT_FAILURE);
    }
}

void condWait(pthread_cond_t *varCond, pthread_mutex_t *mutex) {
	if(pthread_cond_wait(varCond, mutex)) {
        fprintf(stderr, "Error waiting for condition variable\n");
        exit(EXIT_FAILURE);
    }
}


void condSignal(pthread_cond_t *varCond) {
	if(pthread_cond_signal(varCond)) {
		fprintf(stderr, "Error signaling condition variable\n");
		exit(EXIT_FAILURE);
	}
}

int getNumLines(FILE *f) {
    int cnt = 0;
    while (!feof(f))
        if (fgetc(f) == '\n')
            cnt++;
    return cnt;
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
