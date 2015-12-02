#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "list.h"
#include "parshell.h"
#include "terminal_pid_list.h"
#include "commandlinereader.h"

sharedData_t data = NULL;
terminalList_t terminalList;
pthread_t monitorThread;

void *monitorChildren(void *arg) {
    time_t endtime;
    int executionTime;
    int status;
    int pid;

    while(1) {             
        mutexLock(&data->mutex);

		while (data->childCnt == 0 && !data->exited)
			condWait(&data->childCntCond, &data->mutex);

        if (data->exited && data->childCnt == 0) {
            mutexUnlock(&data->mutex);
            pthread_exit(NULL);
        }

        mutexUnlock(&data->mutex);
        pid = wait(&status);

        if (pid == -1)
            perror("Error in wait");

        endtime = time(NULL);

        if (endtime == (time_t) -1) 
            fprintf(stderr, "Error getting child endtime\n");

        mutexLock(&data->mutex);
        update_terminated_process(data->pidList, pid, endtime, status);
        data->childCnt--;

        if (pid != -1) {
            executionTime = get_execution_time(data->pidList, pid);
            fprintf(data->logFile, "iteracao %d\npid: %d ", 
                    data->currentIteration, pid);
            data->currentIteration++;

            if (executionTime != -1) {
            	data->totalRuntime += executionTime;
                fprintf(data->logFile, "execution time: %d s\n", 
                                       executionTime);
            }
            else 
            	fprintf(data->logFile, "execution time: Undetermined\n");

            fprintf(data->logFile, "total execution time: %d s\n",
                                   data->totalRuntime);

         	if (fflush(data->logFile)) 
                perror("Error flushing to file");
        }
        mutexUnlock(&data->mutex);
		condSignal(&data->procLimiterCond);
    }
}


int createProcess(char *argVector[], list_t *pidList) {
    /* Returns 1 if sucessful */
    int pid = fork();
    if (pid < 0) {
        perror("Error forking process");
        return 0;
    }
    else if (pid == 0) {
	int pid = (int) getpid();
	char buffer[BUFFER_SIZE];
	int fd = -1;
	snprintf(buffer, BUFFER_SIZE, "par-shell-out-%d.txt", pid); /* TODO Error checking */
	fclose(stdout); 
	fd = open(buffer, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR); /* TODO ERRR CHEKC*/
        execv(argVector[0], argVector);
        perror("Error executing process");
	close(fd);
        exit(EXIT_FAILURE);
    }
    else {
        time_t starttime = time(NULL);
        if (!insert_new_process(pidList, pid, starttime))
            fprintf(stderr, "Failed to save info for process %d, "
                            "will not display process info on exit\n", pid);
        return 1;
    }
}


void exitShell() {
    mutexLock(&data->mutex);
    data->exited = 1;
    mutexUnlock(&data->mutex);
	condSignal(&data->childCntCond);

    if (pthread_join(monitorThread, NULL))
        fprintf(stderr, "Error waiting for monitoring thread.\n");

    if (fclose(data->logFile))
        perror("Error closing log file");

    lst_print(data->pidList);

    if (pthread_mutex_destroy(&data->mutex))
        fprintf(stderr, "Error destroying mutex.\n");

	if (pthread_cond_destroy(&data->childCntCond) || 
	    pthread_cond_destroy(&data->procLimiterCond))
        fprintf(stderr, "Error destroying condition variables\n");

    killAllPids(terminalList);
    lst_destroy(data->pidList);
    destroyTerminalList(terminalList);
	unlink("/tmp/par-shell-in");
    free(data);
}

void handleSignal(int sig) {
    exitShell();
    exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[]) {

    int i;
    char buffer[BUFFER_SIZE];
    char *argVector[ARGNUM]; 
    data = (sharedData_t) malloc(sizeof(struct sharedData));
    int numLines;
	mkfifo("/tmp/par-shell-in", S_IRUSR | S_IWUSR);
	close(fileno(stdin));
	open("/tmp/par-shell-in", O_RDONLY);


    if (data == NULL) {
        perror("Error allocating space for shared variables in main");
        return EXIT_FAILURE;
    }

    data->logFile = fopen("log.txt", "a+");
    if (data->logFile == NULL) {
        perror("Failed to open log file");
        return EXIT_FAILURE;
    }
    
    numLines = getNumLines(data->logFile);

    if (numLines % 3 != 0) {
        fprintf(stderr, "Log file corrupted, will create a new file.\n");
        data->logFile = freopen("log.txt", "w+", data->logFile);
        if (data->logFile == NULL) {
            perror("Failed to create the new log file");
            return EXIT_FAILURE;
        }
        data->currentIteration = data->totalRuntime = 0;
    }
    else {
        data->currentIteration = numLines / 3; /* there are 3 lines per iteration */
        data->totalRuntime = getTotalRuntime(data->logFile);
    }

    data->pidList = lst_new();
    if (data->pidList == NULL) {
        fprintf(stderr, "Failed to create list to save processes.\n");
        return EXIT_FAILURE;
    }

    terminalList = createPidList();

    if (terminalList == NULL) {
        fprintf(stderr, "Failed to create list to save running terminals.\n");
        return EXIT_FAILURE;
    }

    data->childCnt = 0;
    data->exited = 0; 
    /* Exited issues the exit command to the monitor thread (ie. 1 means par-shell wants to exit) */

    pthread_mutex_init(&data->mutex, NULL);
	pthread_cond_init(&data->childCntCond, NULL);
	pthread_cond_init(&data->procLimiterCond, NULL);

    if (pthread_create(&monitorThread, NULL, monitorChildren, NULL)) {
        fprintf(stderr, "Failed to create thread.\n");
        return EXIT_FAILURE;
    }

    for(i = 0; i < ARGNUM; i++)
        argVector[i] = NULL;

    signal(SIGINT, handleSignal); /* TODO ERROR CHEKC*/

    while (1) {
        int numArgs;
        numArgs = readLineArguments(argVector, ARGNUM, buffer, BUFFER_SIZE);
        if (numArgs < 0) {
            continue;
        }
        else if (numArgs == 0)
            continue;
        if (!strcmp("exit-global", argVector[0])) {
            exitShell();
            return EXIT_SUCCESS;
        }
        else if (!strcmp("new_parshell_terminal", argVector[0])) {
            int pid = atoi(argVector[1]);
            if (!insertPid(pid, terminalList))
                fprintf(stderr, "Error accepting new terminal\n");                
        }
        else if (!strcmp("exiting_parshell_terminal", argVector[0])) {
            int pid = atoi(argVector[1]);
            removePid(pid, terminalList);
        }
        else if (!strcmp("stats", argVector[0])) {
            char *terminalPipePath = argVector[1];
            int terminalPipe_fd = open(terminalPipePath, O_WRONLY);
            mutexLock(&data->mutex);
            write(terminalPipe_fd, &data->childCnt, sizeof(int) / sizeof(char));
            write(terminalPipe_fd, &data->totalRuntime, sizeof(int) / sizeof(char));
            mutexUnlock(&data->mutex);
            close(terminalPipe_fd);
        }
        else {
            mutexLock(&data->mutex);
			while(data->childCnt == MAXPAR)
				condWait(&data->procLimiterCond, &data->mutex);
            if(createProcess(argVector, data->pidList)) {
                data->childCnt++;
				condSignal(&data->childCntCond);
            }
            mutexUnlock(&data->mutex);
        }
    }
    return EXIT_FAILURE; /* This line should not be executed */
}
