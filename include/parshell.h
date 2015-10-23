#ifndef __PARSHELL_H__
#define __PARSHELL_H__

#include <pthread.h>
#include <semaphore.h>
#include "list.h"

#define ARGNUM 7
#define BUFFER_SIZE 128

typedef struct sharedData
{
   int childCnt;
   int exited;
   pthread_mutex_t mutex;
   sem_t sem;
   list_t *pidList;
}* sharedData_t;

/* Cleans up to exit the shell */
void exitShell(sharedData_t data,pthread_t monitorThread);

/* Creates a new process and stores PID and start time in pidList
   Returns 1 on success and 0 on failure */
int createProcess(char *argVector[], list_t *pidList);

/* Function that is run on a separate thread to monitor end times of
   forked processes */
void *monitorChildren(void *arg);

#endif
