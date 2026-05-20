#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#define BUFSIZE 1024

void start_monitor() {
    int pfd[2];
    int hub_pid;
    if ((hub_pid = fork()) < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (hub_pid == 0) {
    char buffer[BUFSIZE];
    int pid;
    if (pipe(pfd) < 0) {
        perror("Pipe failed");
        exit(EXIT_FAILURE);
    }
    if ((pid = fork()) < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        execlp("monitor_reports", "monitor_reports",NULL);
        perror("Execlp failed");
        exit(EXIT_SUCCESS);
    }
    close(pfd[1]);
    int n;
    while ((n = read(pfd[0], buffer, BUFSIZE - 1)) > 0 ) {
        buffer[n] = '\0';
        printf("([MONITOR] %s",buffer);
    }
    close(pfd[0]);
   }

}

void calculate_scores(char* districtList) {
    char* district = strtok(districtList, " ");
    while (district != NULL) {
        int pfd[2];
        if (pipe(pfd) < 0) {
            printf("Pipe failed");
            continue;
        }
       int scorer_pid;
        if ((scorer_pid = fork()) < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        if (scorer_pid == 0) {
            close(pfd[0]);
            dup2(pfd[1], STDOUT_FILENO);
            close(pfd[1]);
            execlp("scorer", "scorer", NULL);
            perror("Execlp failed");
            exit(EXIT_SUCCESS);
        }else {
            close(pfd[1]);
            char buffer[BUFSIZE];
            int read_scores = read(pfd[0], buffer, BUFSIZE - 1);
            if (read_scores > 0) {
                buffer[read_scores] = '\0';
                printf("%s", buffer);
            }
            close(pfd[0]);
            waitpid(scorer_pid, NULL, 0);
        }
        district = strtok(NULL, " ");
    }
}

