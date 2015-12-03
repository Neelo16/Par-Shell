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

sharedData_t data;
terminalList_t terminalList;
pthread_t monitorThread;
int control_open_fd;
sigset_t blockSIGINTSet; 

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
        int pid = getpid();
        char buffer[BUFFER_SIZE];
        int fd = -1;

        if (snprintf(buffer, BUFFER_SIZE, "par-shell-out-%d.txt", pid) < 0)
            fprintf(stderr, "Error creating process output filename, "
                            "will not redirect output\n");
        else { 

            fclose(stdout); 
            fd = open(buffer, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR); 

            if (fd < 0) {
                perror("Error opening process output file");
                exit(EXIT_FAILURE);
            }
        }

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

void *processForkRequest(void *args) {
    char **argv = (char **) args;

    if (argv == NULL) 
        pthread_exit(NULL);

    mutexLock(&data->mutex);
    while(data->childCnt == MAXPAR)
        condWait(&data->procLimiterCond, &data->mutex);
    if(createProcess(argv, data->pidList)) {
        data->childCnt++;
        condSignal(&data->childCntCond);
    }
    mutexUnlock(&data->mutex);

    while (*argv != NULL) {
        free(*argv++);
    }

    free( (char**) args);

    pthread_exit(NULL);
}

void exitShell() {
    killAllPids(terminalList);
    mutexLock(&data->mutex);
    data->exited = 1;
    mutexUnlock(&data->mutex);
    condSignal(&data->childCntCond);

    if (pthread_join(monitorThread, NULL))
        fprintf(stderr, "Error waiting for monitoring thread.\n");

    if (fclose(data->logFile))
        perror("Error closing log file");

    if (pthread_mutex_destroy(&data->mutex))
        fprintf(stderr, "Error destroying mutex.\n");

    if (pthread_cond_destroy(&data->childCntCond) || 
        pthread_cond_destroy(&data->procLimiterCond))
        fprintf(stderr, "Error destroying condition variables\n");

    if (close(control_open_fd))
        perror("Error closing pipe");

    if (unlink(PARSHELL_PIPE_PATH))
        perror("Error unlinking pipe");

    lst_print(data->pidList);

    lst_destroy(data->pidList);
    destroyTerminalList(terminalList);
    free(data);
    exit(EXIT_SUCCESS);
}

void handleSignal(int sig) {
    exitShell();
}

int main(int argc, char const *argv[]) {

    int numLines;
    char buffer[BUFFER_SIZE];
    char *argVector[ARGNUM];

    data = (sharedData_t) malloc(sizeof(struct sharedData));

    unlink(PARSHELL_PIPE_PATH); /* makes sure we don't have a leftover */
                                /* pipe from a previous instance of    */
                                /* par-shell                           */

    if (mkfifo(PARSHELL_PIPE_PATH, S_IRUSR | S_IWUSR)) {
        perror("Error creating pipe");
        return EXIT_FAILURE;
    }

    close(fileno(stdin));
    if (open(PARSHELL_PIPE_PATH, O_RDONLY)) {
        perror("Error replacing stdin with pipe");
        return EXIT_FAILURE;
    }

    control_open_fd = open(PARSHELL_PIPE_PATH, /* file descriptor used to  */
                            O_WRONLY);         /* prevent the pipe from    */
                                               /* becoming invalid when no */
                                               /* terminals are writing to */
    if (control_open_fd < 0)                   /* it                       */
        perror("Error opening pipe");

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
    /* Exited issues the exit command to the monitor thread */
    /*        (ie. 1 means par-shell wants to exit)         */

    pthread_mutex_init(&data->mutex, NULL);
    pthread_cond_init(&data->childCntCond, NULL);
    pthread_cond_init(&data->procLimiterCond, NULL);

    /* Blocks the SIGINT signal in the thread signal mask  */
    /* to prevent the monitor thread from trying to handle */
    /* that signal.                                        */
    sigemptyset(&blockSIGINTSet);
    sigaddset(&blockSIGINTSet, SIGINT);
    pthread_sigmask(SIG_BLOCK, &blockSIGINTSet, NULL);

    if (pthread_create(&monitorThread, NULL, monitorChildren, NULL)) {
        fprintf(stderr, "Failed to create thread.\n");
        return EXIT_FAILURE;
    }

    /* Unblocks SIGINT again so the main thread can handle it  */
    /* properly                                                */
    pthread_sigmask(SIG_UNBLOCK, &blockSIGINTSet, NULL);

    if (signal(SIGINT, handleSignal) == SIG_ERR)
        perror("Error setting handler for SIGINT, will proceed "
               "without handling the signal");

    while (1) {
        int numArgs;
        numArgs = readLineArguments(argVector, ARGNUM, buffer, BUFFER_SIZE);
        if (numArgs < 0) {
            fprintf(stderr, "Error reading arguments\n");
            exitShell();
        }
        if (numArgs == 0)
            continue;
        if (!strcmp("exit-global", argVector[0])) {
            exitShell();
        }
        else if (!strcmp("new_parshell_terminal", argVector[0])) {
            int pid = atoi(argVector[1]);
            if (!insertPid(pid, terminalList)) {
                fprintf(stderr, "Error accepting new terminal\n");                
                kill(pid, SIGKILL);
            }
        }
        else if (!strcmp("exiting_parshell_terminal", argVector[0])) {
            int pid = atoi(argVector[1]);
            removePid(pid, terminalList);
        }
        else if (!strcmp("stats", argVector[0])) {
            char *terminalPipePath = argVector[1];
            int terminalPipe_fd = open(terminalPipePath, O_WRONLY);

            if (terminalPipe_fd < 0) {
                perror("Error opening pipe");
                continue;
            }

            mutexLock(&data->mutex);
            if (write(terminalPipe_fd, &data->childCnt, sizeof(int)) < 0 ||
                write(terminalPipe_fd, &data->totalRuntime, sizeof(int)) < 0)
                perror("Error sending stats to par-shell-terminal");

            mutexUnlock(&data->mutex);

            if (close(terminalPipe_fd) < 0)
                perror("Error closing pipe");
        }
        else {
            pthread_t processingThread;
            pthread_attr_t attr;
            char **argVectorCopy = copyStringVector(argVector, 
                                                    numArgs);

            if (argVectorCopy == NULL) {
                fprintf(stderr, "Error copying arguments for request "
                                "processing, will not create process\n");
                continue;
            }

                /* A detached thread is created to deal with  */
                /* the launching of child processes.          */
                /* If par-shell is busy the thread is blocked */

            if (pthread_attr_init(&attr)) {
                fprintf(stderr, "Error setting attributes for "
                                "request processing, will not "
                                "create process\n");
                continue;
            }

            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            /* Just like when creating the monitor thread, we */
            /* block SIGINT for the created thread so that it */
            /* is handled only by the main thread             */
            pthread_sigmask(SIG_BLOCK, &blockSIGINTSet, NULL);

            if (pthread_create(&processingThread,       
                               &attr,                   
                               processForkRequest,   
                               (void*) argVectorCopy))
                fprintf(stderr, "Error creating thread for request, "
                                "will not create process\n");;

            pthread_sigmask(SIG_UNBLOCK, &blockSIGINTSet, NULL);

            if (pthread_attr_destroy(&attr))
                fprintf(stderr, "Error destroying pthread attributes\n");

        }
    }

    return EXIT_FAILURE; /* This line should not be executed */
}
