#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "util.h"

int getTotalRuntime(FILE *f) {
    int totalRuntime = 0;
    char c;

    if (fgetc(f) == EOF)
      return 0;

    fseek(f, -4, SEEK_END); /* Sets the file position indicator to the last
                               digit in the total runtime number */
    while ((c = fgetc(f)) != ' ') {
      
      fseek(f, -2, SEEK_CUR); /* Undoes the position indicator increment
                                 cause by fgetc and seeks to the previous
                                 character */
    }

    fscanf(f, "%d", &totalRuntime);

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
    while (!feof(f))
        if (fgetc(f) == '\n')
            cnt++;
    return cnt;
}
