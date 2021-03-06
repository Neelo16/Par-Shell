#ifndef __PARSHELL_H__
#define __PARSHELL_H__

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include "list.h"
#include "terminal_pid_list.h"

#define ARGNUM 7
#define BUFFER_SIZE 512
#define MAXPAR 4
#define PARSHELL_PIPE_PATH "/tmp/par-shell-in"

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
 * Returns 1 on success and 0 on failure 
 */
int createProcess(char *argVector[], list_t *pidList);

/* Function used by a thread to process a request to create a
 * new child process. The argument should be a vector of char*
 * containing the process pathname in the first position, and
 * any arguments in the remaining positions, ending with a
 * NULL pointer.
 */
void *processForkRequest(void *args);

/* Function that runs on a separate thread to monitor end times of
 * forked processes 
 */
void *monitorChildren(void *arg);

#endif
