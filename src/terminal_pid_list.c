#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "terminal_pid_list.h"

terminalList_t createPidList() {
    terminalList_t list = (terminalList_t) malloc(sizeof(struct terminalList));
    if (list != NULL) 
        list->first = NULL;
    return list;
}

int insertPid(int pid, terminalList_t list) {
    terminalListItem_t next;
    terminalListItem_t item;

    if (list == NULL) 
        return 0;

    next = list->first;
    item = (terminalListItem_t) malloc(sizeof(struct terminalListItem));

    if(item != NULL) {
        item->pid = pid;
        item->next = next;
    }
    else 
        return 0;

    list->first = item;
    
    return 1;
}

void removePid(int pid, terminalList_t list) {
    terminalListItem_t item;
    terminalListItem_t aux;

    if (list == NULL || list->first == NULL) 
        return;

    if(list->first->pid == pid) {
        aux = list->first;
        list->first = aux->next;
        free(aux);
        return;
    }

    item = list->first;

    while (item->next != NULL && item->next->pid != pid)
        item = item->next;

    if(item->next == NULL) 
        return;

    aux = item->next;
    item->next = aux->next;
    free(aux);
}

void killAllPids(terminalList_t list) {
    terminalListItem_t item;

    if (list == NULL) 
        return;

    item = list->first;

    while (item != NULL) {
        if (kill(item->pid, SIGKILL))
            perror("Error killing process");
        item = item->next;
    }
}

void destroyTerminalList(terminalList_t list) {
    terminalListItem_t item;

    if(list == NULL) 
        return;

    item = list->first;
    free(list);

    while(item != NULL) {
        terminalListItem_t aux = item;
        item = item->next;
        free(aux);
    }
}