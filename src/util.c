#include <stdio.h>
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