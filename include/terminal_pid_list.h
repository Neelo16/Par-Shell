#ifndef __TERMINAL_PID_H__
#define __TERMINAL_PID_H__

typedef struct terminalListItem {
	int pid;
	terminalListItem *next;
}* terminalList_t;

/* Create a new list for storing pids. Returns NULL on error */
terminalList_t createList();

/* Insert a pid in the given list. Returns 0 on success, and a
 * non-zero value on error.
 */
int insertPid(int pid, terminalList_t list);

/** Remove the given pid from the list. */
void removePid(int pid, terminalList_t list);

/** Send a KILL signal to all pids in the list. */
void killAllPids(terminalList_t list);

/** Destroy the given list. */
void destroyTerminalList(terminalList_t list);

#endif
