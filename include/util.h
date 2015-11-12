#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <pthread.h>

/* Parses given file and extracts total runtime from the last line.
   Given file's position indicator will be set to the end of the file */
int getTotalRuntime(FILE *f);

/* Locks the mutex and exits if there's an error */
void mutexLock(pthread_mutex_t *mutex);

/* Unlocks the mutex and exits if there's an error */
void mutexUnlock(pthread_mutex_t *mutex);

/* Waits for the condition variable cond (unlocking its mutex) and exits on error */
void condWait(pthread_cond_t *cond, pthread_mutex_t *mutex);

/* Signal condition variable cond and exits on error */
void condSignal(pthread_cond_t *cond);

/* Reads given file and returns number of lines from current seek position */
int getNumLines(FILE *f);

#endif
