#ifndef __PARSHELL_H__
#define __PARSHELL_H__

#include <pthread.h>
#include <semaphore.h>
#include "list.h"
#include <stdio.h>

#define ARGNUM 7
#define BUFFER_SIZE 128
#define MAXPAR 4

typedef struct sharedData
{
   int childCnt;
   int exited;
   pthread_mutex_t mutex;
   pthread_cond_t procLimiterCond;
   pthread_cond_t childCntCond;
   list_t *pidList;
   int totalRuntime;
   int currentIteration;
   FILE *logFile;
}* sharedData_t;

/* Cleans up to exit the shell */
void exitShell(sharedData_t data,pthread_t monitorThread);

/* Creates a new process and stores PID and start time in pidList
   Returns 1 on success and 0 on failure */
int createProcess(char *argVector[], list_t *pidList);

/* Locks the mutex and exits if there's an error */
void mutexLock(pthread_mutex_t *mutex);

/* Unlocks the mutex and exits if there's an error */
void mutexUnlock(pthread_mutex_t *mutex);

/* Waits for the semaphore sem and exits on error */
void semWait(sem_t *sem);

/* Post the semaphore sem and exits on error */
void semPost(sem_t *sem);

/* Reads given file and returns number of lines from current seek position */
int getNumLines(FILE *f);

/* Function that is run on a separate thread to monitor end times of
   forked processes */
void *monitorChildren(void *arg);

#endif
