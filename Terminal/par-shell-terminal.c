#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "par-shell-terminal.h"

int main(int argc, char const *argv[])
{
    int pipe_fd;
    int pid = getpid();
    int commandLength = 0;
    char command[BUFFER_SIZE];
    char pipePathName[BUFFER_SIZE];
    memset(command, 0x0, BUFFER_SIZE);

    if (argc < 2) {
        puts("Usage: par-shell-terminal <path to pipe>");
        return EXIT_FAILURE;
    }

    if  (snprintf(pipePathName, 
                             BUFFER_SIZE, 
                             "/tmp/par-shell-terminal-%d", 
                             pid) < 0) {
        fprintf(stderr, "Error starting terminal, exiting...\n");
        return EXIT_FAILURE;
    }

    pipe_fd = open(argv[1], O_WRONLY);

    if (pipe_fd < 0) {
        perror("Error opening given pipe");
        return EXIT_FAILURE;
    }

    commandLength = snprintf(command, 
                             BUFFER_SIZE, 
                             "new_parshell_terminal %d\n", 
                             pid);

    if ((commandLength < 0) || (write(pipe_fd, command, commandLength) < 0)) {
        fprintf(stderr, "Error informing par-shell of new terminal\n");
        return EXIT_FAILURE;
    }

    while (1) {
        if (fgets(command, BUFFER_SIZE, stdin) == NULL) {
            fprintf(stderr,"Error reading input\n");
            return EXIT_FAILURE;
        }

        if (!strcmp(command, "stats\n")) {
            int stats_fd;
            int numChildrenRun = 0;
            int totalExec = 0;

            unlink(pipePathName);

            if (mkfifo(pipePathName, S_IRUSR | S_IWUSR)) {
                perror("Error creating pipe, will not print stats");
                continue;
            }

            commandLength = snprintf(command, BUFFER_SIZE, 
                                     "stats %s\n", 
                                     pipePathName);

            if (commandLength < 0) {
                fprintf(stderr, "Error getting stats");
                continue;
            }

            if (write(pipe_fd, command, commandLength) < 0) {
                perror("Error asking par-shell for stats");
                continue;
            }

            stats_fd = open(pipePathName, O_RDONLY);

            if (stats_fd < 0) {
                perror("Error opening pipe for communication with "
                       "main par-shell");
                continue;
            }

            printf("Running processes: ");
            if (read(stats_fd, &numChildrenRun, sizeof(int)) < 0)
                puts("Undetermined");
            else
                printf("%d\n", numChildrenRun);

            printf("Total Execution Time: ");
            if (read(stats_fd, &totalExec, sizeof(int)) < 0)
                puts("Undetermined");
            else
                printf("%d\n", totalExec);
            
            if (close(stats_fd))
                perror("Error closing pipe");

            if (unlink(pipePathName))
                perror("Error unlinking pipe");
        }

        else if (!strcmp(command, "exit\n")) {

            commandLength = snprintf(command, BUFFER_SIZE, 
                                     "exiting_parshell_terminal %d\n",
                                      pid);

            if (commandLength < 0 || write(pipe_fd, command, commandLength) < 0)
                perror("Error informing par-shell about terminal termination");

            if (close(pipe_fd))
                perror("Error closing pipe");

            return EXIT_SUCCESS;
        }

        else {
            commandLength = strlen(command);
            if (write(pipe_fd, command, commandLength) < 0)
                perror("Error sending command to par-shell");
        }
    }

    return EXIT_FAILURE; /* This line should not be executed */
}