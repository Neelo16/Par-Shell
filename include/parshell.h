#ifndef __PARSHELL_H__
#define __PARSHELL_H__

#include <pthread.h>
#include <semaphore.h>
#include "list.h"

#define ARGNUM 7
#define BUFFER_SIZE 80

typedef struct sharedData
{
   int childCnt;
   int exited;
   pthread_mutex_t mutex;
   sem_t sem;
   list_t *pidList;
}* sharedData_t;

int createProcess(char *argVector[], list_t *pidList);
void *monitorChildren(void *data);

#endif
