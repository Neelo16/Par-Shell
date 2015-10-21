#ifndef __PARSHELL_H__
#define __PARSHELL_H__

#include <pthread.h>
#include "list.h"

#define ARGNUM 7

typedef struct
{
   int childCnt;
   pthread_mutex_t mutex;
   list_t pidList;
}* sharedData;

void exitShell(int childCnt);
int createProcess(char *argVector[]);

#endif
