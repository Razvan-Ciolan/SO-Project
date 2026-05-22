#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#define PID_FILE ".monitor_pid"

volatile sig_atomic_t keep_running = 1;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        const char msg[] = "\nSIGINT received. Deleting PID file and shutting down...\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
        keep_running = 0;
    } else if (sig == SIGUSR1) {
        const char msg[] = "\nSignal SIGUSR1 received: A new report has been added to the system!\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    }
}

int main() {
    FILE* f = fopen(PID_FILE, "r");
    if (f != NULL) {
        int existing_pid;
        if (fscanf(f, "%d", &existing_pid) == 1) {
            if (kill(existing_pid, 0) == 0) {
                char err_msg[128];
                int len = sprintf(err_msg, "ERR: PID file already running with PID:%d.\n", existing_pid);
                write(STDOUT_FILENO, err_msg, len);
                fclose(f);
                return 1;
            }
        }
        fclose(f);
    }

    int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error creating PID file");
        exit(EXIT_FAILURE);
    }
    char pid_str[16];
    int len = snprintf(pid_str, sizeof(pid_str), "%d", getpid());
    if (write(fd, pid_str, len) < 0) {
        perror("Error writing to PID file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    printf("Started with PID: %d. Listening for signals...\n", getpid());

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        unlink(PID_FILE);
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error setting up SIGUSR1 handler");
        unlink(PID_FILE);
        exit(EXIT_FAILURE);
    }

    while (keep_running) {
        pause();
    }

    if (unlink(PID_FILE) == 0) {
        printf("Cleanup successful: %s deleted.\n", PID_FILE);
    } else {
        perror("Error deleting PID file");
    }

    return 0;
}
