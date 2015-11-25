#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"

int getTotalRuntime(FILE *f) {
    int totalRuntime = 0;

    rewind(f); /* Ensures the position indicator starts at 
                  the beginning of the file so we can properly
                  detect an empty file */

    if (fgetc(f) == EOF)
      return 0;

    fseek(f, -4L, SEEK_END); /* Sets the file position indicator to the last
                               digit in the total runtime number */
    while (fgetc(f) != ' ')
      fseek(f, -2L, SEEK_CUR); /* Undoes the position indicator increment
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
    int c;
    rewind(f); /* Makes sure we count every line in the file */
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n')
            cnt++;
    }

    if (ftell(f) == 0)
        return 0;
    else if (ftell(f) > 0 && cnt == 0) /* if there's at least one 
                                        * character in the file,
                                        * it counts as a line */
        return 1;

    fseek(f, -1L, SEEK_END);

    return fgetc(f) == '\n'? cnt : cnt + 1; /* Checks if there's a line at the end
                                               without a newline character */
}

void readFromPipe(int fd, char *buffer, int buffersize) {
    char c = '\n';
    while (buffersize-- > 0 && c != '\0') {
        read(fd, &c, 1);
        *buffer++ = c;
    }
}