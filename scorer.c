#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_STR 100

typedef struct {
    int id;
    char inspector[MAX_STR];
    double lat, lon;
    char category[MAX_STR];
    int severity;
    time_t timestamp;
    char description[MAX_STR];
} Report;

typedef struct {
    char name[MAX_STR];
    int score;
} InspectorScore;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: scorer <district>\n");
        exit(EXIT_FAILURE);
    }

    char *district = argv[1];
    char path[150];
    sprintf(path, "%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("No reports found.\n");
        exit(EXIT_FAILURE);
    }

    Report report;
    InspectorScore scores[150];
    int inspectorNum = 0;

    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        int found = 0;
        for (int i = 0; i < inspectorNum; i++) {
            if (strcmp(scores[i].name, report.inspector) == 0) {
                scores[i].score += report.severity;
                found = 1;
                break;
            }
        }

        // Verificăm și limita array-ului pentru a preveni buffer overflow
        if (found != 1 && inspectorNum < 150) {
            strcpy(scores[inspectorNum].name, report.inspector);
            scores[inspectorNum].score = report.severity;
            inspectorNum++;
        }
    }
    close(fd);

    // Tot ce se printează aici se duce automat în pipe-ul din city_hub
    for (int i = 0; i < inspectorNum; i++) {
        printf("%s = %d\n", scores[i].name, scores[i].score);
    }

    return 0;
}
