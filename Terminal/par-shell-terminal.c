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
    memset(command, 0x0, BUFFER_SIZE);

    if (argc < 2) {
        puts("Usage: par-shell-terminal <path to pipe>");
        return EXIT_FAILURE;
    }

    pipe_fd = open(argv[1], O_WRONLY);

    commandLength = snprintf(command, BUFFER_SIZE, "new_parshell_terminal %d\n", pid);
    write(pipe_fd, command, commandLength); /* :( */

    if (pipe_fd < 0) {
        perror("Error opening the pipe");
        return EXIT_FAILURE;
    }

    while (1) {
        fgets(command, BUFFER_SIZE, stdin);

        if (!strcmp(command, "stats\n")) {
            int stats_fd;
            int numChildrenRun = 0;
            int totalExec = 0;
            char pipePathName[BUFFER_SIZE];

            snprintf(pipePathName, BUFFER_SIZE, "/tmp/%d", pid);
            mkfifo(pipePathName, S_IRUSR | S_IWUSR);

            commandLength = snprintf(command, BUFFER_SIZE, 
                                     "stats %s\n", 
                                     pipePathName);

            write(pipe_fd, command, commandLength); /* TODO ERROR CHEKC */
            stats_fd = open(pipePathName, O_RDONLY);
            read(stats_fd, &numChildrenRun, sizeof(int) / sizeof(char));
            read(stats_fd, &totalExec, sizeof(int) / sizeof(char));
            printf("Running processes: %d\n", numChildrenRun);
            printf("Total Execution Time: %d\n", totalExec);
            close(stats_fd);
            unlink(pipePathName);
        }

        else if (!strcmp(command, "exit\n")) {
            
            commandLength = snprintf(command, BUFFER_SIZE, 
                                     "exiting_parshell_terminal %d\n",
                                      pid);

            write(pipe_fd, command, commandLength); /* You know what to do */
            close(pipe_fd);
            return EXIT_SUCCESS;
        }

        else {
            commandLength = strlen(command);
            write(pipe_fd, command, commandLength);
        }
    }

    return EXIT_FAILURE; /* This line should not be executed */
}