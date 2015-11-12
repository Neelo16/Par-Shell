#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "util.h"

int getTotalRuntime(FILE *f) {
    int totalRuntime = 0;

    rewind(f); /* Ensures the position indicator starts at 
                  the beginning of the file so we can properly
                  detect an empty file */

    if (fgetc(f) == EOF)
      return 0;

    fseek(f, -4, SEEK_END); /* Sets the file position indicator to the last
                               digit in the total runtime number */
    while (fgetc(f) != ' ')
      fseek(f, -2, SEEK_CUR); /* Undoes the position indicator increment
                                 caused by fgetc and seeks to the previous
                                 character */

    fscanf(f, "%d", &totalRuntime);

    fseek(f, 0, SEEK_END);

    return totalRuntime;
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
    rewind(f); /* Makes sure we count every line in the file */
    while (!feof(f))
        if (fgetc(f) == '\n')
            cnt++;
    return cnt;
}
