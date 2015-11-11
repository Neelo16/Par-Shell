/*
 * list.c - implementation of the integer list functions 
 */


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "list.h"



list_t* lst_new()
{
    list_t *list;
    list = (list_t*) malloc(sizeof(list_t));
    if (list != NULL)
        list->first = NULL;
    return list;
}


void lst_destroy(list_t *list)
{
    struct lst_iitem *item, *nextitem;

    item = list->first;
    while (item != NULL){
        nextitem = item->next;
        free(item);
        item = nextitem;
    }
    free(list);
}


int insert_new_process(list_t *list, int pid, time_t starttime)
{
    lst_iitem_t *item;
    item = (lst_iitem_t *) malloc (sizeof(lst_iitem_t));
    if (item != NULL){
        item->pid = pid;
        item->starttime = starttime;
        item->endtime = 0;
        item->status = 0;
        item->next = list->first;
        list->first = item;
        return 1;
    }
    return 0; /* If it failed to alocate memory to the new item */
}


void update_terminated_process(list_t *list, int pid, time_t endtime, int status)
{
    lst_iitem_t* item;
    if (list == NULL) return;
    item = list->first;
    while (item != NULL && item->pid != pid)
        item = item->next;
    if (item != NULL) {
        item->endtime = endtime;
        item->status = status;
    }
}

int get_execution_time(list_t *list, int pid) {
    lst_iitem_t* item;
    if (list == NULL) return -1;
    item = list->first;
    while (item != NULL) {
        if (item->pid == pid)
            return difftime(item->endtime, item->starttime);
        item = item->next;
    }
    return -1;
}

void lst_print(list_t *list)
{
    lst_iitem_t *item;

    item = list->first;
    while (item != NULL){
        double executionTime = difftime(item->endtime, item->starttime);
        int status = item->status;
        int pid = item->pid;
        if (pid != -1)
        {
            printf("PID: %d\t", pid);
            if (executionTime < 0)
                puts("TIME: Undetermined\t");
            else
                printf("TIME: %03.0f seconds\t", executionTime);
            if (WIFEXITED(status))
                printf("EXIT STATUS: %d\n", WEXITSTATUS(status));
            else
                printf("EXIT STATUS: N/A (improper termination)\n");
        }

        item = item->next;
    }
}
