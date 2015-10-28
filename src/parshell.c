#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "list.h"
#include "parshell.h"
#include "commandlinereader.h"


void *monitorChildren(void *arg){
    sharedData_t data = (sharedData_t) arg;
    time_t endtime;
    int status;
    int pid;
    while(1) {
        sem_wait(&data->sem);               
        mutex_lock(&data->mutex);
        if(data->childCnt == 0 && data->exited){
            mutex_unlock(&data->mutex);
            pthread_exit(NULL);
        }
        mutex_unlock(&data->mutex);
        pid = wait(&status);
        if (pid == -1)
            perror("Error in wait");
        endtime = time(NULL);
        if(endtime == (time_t) -1) 
            fprintf(stderr, "Error getting child endtime\n");

        mutex_lock(&data->mutex);
        update_terminated_process(data->pidList, pid, endtime, status);
        data->childCnt--;
        mutex_unlock(&data->mutex);
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
    mutex_lock(&data->mutex);
    data->exited = 1;
    mutex_unlock(&data->mutex);

    sem_post(&data->sem); /* Unlocks monitor thread in order to complete exit procedures */

    if (pthread_join(monitorThread, NULL))
        fprintf(stderr, "Error waiting for monitoring thread\n");
    lst_print(data->pidList);

    if (pthread_mutex_destroy(&data->mutex))
        fprintf(stderr, "Error destroying mutex\n");

    if (sem_destroy(&data->sem))
        perror("Error destroying semaphore");

    lst_destroy(data->pidList);
    free(data);
}

void mutex_lock(pthread_mutex_t *mutex) {
    if (pthread_mutex_lock(mutex)) {
        perror("Error locking the mutex");
        exit(EXIT_FAILURE);
    }
}

void mutex_unlock(pthread_mutex_t *mutex) {
    if (pthread_mutex_unlock(mutex)) {
        perror("Error unlocking the mutex");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[]) {

    int i;
    char buffer[BUFFER_SIZE];
    char *argVector[ARGNUM]; 
    sharedData_t data = malloc(sizeof(struct sharedData));
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

    if (sem_init(&data->sem, 0, 0)) { 
    /* Semaphore used to lock monitor thread while there are no running children */
        perror("Failed to initialize semaphore");
        return EXIT_FAILURE;
    }

    if (pthread_create(&monitorThread, NULL, monitorChildren, (void*) data)) {
        fprintf(stderr, "Failed to create thread.\n");
        return EXIT_FAILURE;
    }

    for(i = 0; i < ARGNUM; i++)
        argVector[i] = NULL;

    while (1) {
        int numArgs;
        numArgs = readLineArguments(argVector, ARGNUM, buffer, BUFFER_SIZE);
        if (numArgs < 0)
        {
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
            mutex_lock(&data->mutex);
            if(createProcess(argVector, data->pidList)) {
                data->childCnt++;
                sem_post(&data->sem);
            }
            mutex_unlock(&data->mutex);
        }
    }

    
    return EXIT_FAILURE; /* This line should not be executed */
}
