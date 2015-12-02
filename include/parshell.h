#ifndef __PARSHELL_H__
#define __PARSHELL_H__

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include "list.h"
#include "terminal_pid_list.h"

#define ARGNUM 7
#define BUFFER_SIZE 128
#define MAXPAR 4

typedef struct sharedData
{
   int exited;
   int childCnt;
   int totalRuntime;
   int currentIteration;
   pthread_cond_t procLimiterCond;
   pthread_cond_t childCntCond;
   pthread_mutex_t mutex;
   list_t *pidList;
   FILE *logFile;
}* sharedData_t;

/* Cleans up to exit the shell */
void exitShell();

/* Creates a new process and stores PID and start time in pidList
   Returns 1 on success and 0 on failure */
int createProcess(char *argVector[], list_t *pidList);

/* Creates a new thread to process a request to create a
 * new child process.
 */
void *processForkRequest(void *args);

/* Function that is run on a separate thread to monitor end times of
   forked processes */
void *monitorChildren(void *arg);

#endif
