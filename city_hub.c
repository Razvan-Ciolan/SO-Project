#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFSIZE 1024

void start_monitor() {
    int pfd[2];
    int hub_pid;

    // Pipe-ul trebuie creat înainte de fork
    if (pipe(pfd) < 0) {
        perror("Pipe failed");
        return;
    }

    if ((hub_pid = fork()) < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (hub_pid == 0) {
        char buffer[BUFSIZE];
        int pid;

        if ((pid = fork()) < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            close(pfd[0]);
            dup2(pfd[1], STDOUT_FILENO);
            close(pfd[1]);

            execlp("./monitor_reports", "monitor_reports", NULL);
            perror("Execlp failed");
            exit(EXIT_FAILURE);
        }

        close(pfd[1]);
        int n;
        while ((n = read(pfd[0], buffer, BUFSIZE - 1)) > 0 ) {
            buffer[n] = '\0';
            if (strncmp(buffer, "ERR:", 4) == 0) {
                printf("\n[HUB ALERT] Monitor stopped: %s\n", buffer + 4);
            } else {
                printf("\n[MONITOR LOG] %s", buffer);
            }
            fflush(stdout);
        }
        close(pfd[0]);
        waitpid(pid, NULL, 0);
        printf("\n[HUB INFO] Monitor closed.\n");
        // Oprim procesul din background ca să nu revină în main
        exit(EXIT_SUCCESS);
    }

    // Părintele închide pipe-urile pe care nu le folosește
    close(pfd[0]);
    close(pfd[1]);
    printf("Background monitor started (PID: %d)\n", hub_pid);
}

void calculate_scores(char* districtList) {
    // Copiem string-ul ca strtok să nu strice datele
    char copy[512];
    strncpy(copy, districtList, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    printf("\n--- COMBINED WORKLOAD REPORT ---\n");
    char* district = strtok(copy, " ");

    while (district != NULL) {
        int pfd[2];
        if (pipe(pfd) < 0) {
            perror("Pipe failed");
            district = strtok(NULL, " ");
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

            // Trimitem districtul ca argument către programul scorer
            execlp("./scorer", "scorer", district, NULL);
            perror("Execlp failed");
            exit(EXIT_FAILURE);
        } else {
            close(pfd[1]);
            char buffer[BUFSIZE];
            int read_scores;

            printf("\n[District: %s]\n", district);
            while ((read_scores = read(pfd[0], buffer, BUFSIZE - 1)) > 0) {
                buffer[read_scores] = '\0';
                printf("%s", buffer);
            }
            close(pfd[0]);
            waitpid(scorer_pid, NULL, 0);
        }
        district = strtok(NULL, " ");
    }
    printf("--------------------------------\n");
}

int main() {
    char command[256];
    char args[256];

    printf("Welcome to City Hub Shell!\n");
    while (1) {
        printf("hub_shell> ");
        fflush(stdout);

        if (scanf("%s", command) <= 0) break;

        if (strcmp(command, "start_monitor") == 0) {
            start_monitor();
        }
        else if (strcmp(command, "calculate_scores") == 0) {
            char c; while((c = getchar()) == ' ' || c == '\t');
            int i = 0;
            while (c != '\n' && c != EOF && i < 255) {
                args[i++] = c;
                c = getchar();
            }
            args[i] = '\0';

            calculate_scores(args);
        }
        else if (strcmp(command, "exit") == 0) {
            break;
        }
        else {
            printf("Unknown command. Try: start_monitor, calculate_scores <list>, exit\n");
            char c; while((c = getchar()) != '\n' && c != EOF); // clear buffer
        }
    }
    return 0;
}
