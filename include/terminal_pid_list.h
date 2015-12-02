#ifndef __TERMINAL_PID_H__
#define __TERMINAL_PID_H__

typedef struct terminalListItem {
	int pid;
	struct terminalListItem *next;
}* terminalListItem_t;

typedef struct terminalList {
    terminalListItem_t first;
}* terminalList_t;

/* Create a new list for storing pids. Returns NULL on error */
terminalList_t createPidList();

/* Insert a pid in the given list. Returns 0 on failure, and a
 * non-zero value otherwise.
 */
int insertPid(int pid, terminalList_t list);

/* Remove the given pid from the list. */
void removePid(int pid, terminalList_t list);

/* Send a KILL signal to all pids in the list. */
void killAllPids(terminalList_t list);

/* Destroy the given list. */
void destroyTerminalList(terminalList_t list);

#endif
