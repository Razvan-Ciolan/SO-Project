#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#define MAX_STR 100

typedef struct {
    int id;
    char inspector[MAX_STR];
    double lat, lon;
    char categorie[MAX_STR];
    int gravitate;
    time_t data;
    char descriere[MAX_STR];
} Raport;

// --- PERMISIUNI (Bit-to-Symbol manual) ---

void perm_to_str(mode_t m, char *s) {
    s[0] = (m & S_IRUSR) ? 'r' : '-'; s[1] = (m & S_IWUSR) ? 'w' : '-'; s[2] = (m & S_IXUSR) ? 'x' : '-';
    s[3] = (m & S_IRGRP) ? 'r' : '-'; s[4] = (m & S_IWGRP) ? 'w' : '-'; s[5] = (m & S_IXGRP) ? 'x' : '-';
    s[6] = (m & S_IROTH) ? 'r' : '-'; s[7] = (m & S_IWOTH) ? 'w' : '-'; s[8] = (m & S_IXOTH) ? 'x' : '-';
    s[9] = '\0';
}

// Verifică rolul față de permisiunile reale ale fișierului
int are_permisiune(const char *cale, const char *rol, mode_t bit_necesar) {
    struct stat st;
    if (stat(cale, &st) != 0) return 1; // Permitem dacă fișierul nu există încă

    if (strcmp(rol, "manager") == 0) return (st.st_mode & (bit_necesar << 6)); // Verifică biții de Owner
    if (strcmp(rol, "inspector") == 0) return (st.st_mode & (bit_necesar << 3)); // Verifică biții de Group
    return 0;
}

// --- LOGGING ---

void log_action(const char *dist, const char *rol, const char *user, const char *msg) {
    char cale[256]; sprintf(cale, "%s/logged_district", dist);

    // Verifică permisiunea de scriere (Manager only conform schemei pentru scriere)
    if (access(cale, F_OK) == 0 && strcmp(rol, "manager") != 0) return;

    int fd = open(cale, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        chmod(cale, 0644);
        time_t acum = time(NULL);
        char *t = ctime(&acum); t[strlen(t)-1] = '\0';
        dprintf(fd, "[%s] %s (%s): %s\n", t, user, rol, msg);
        close(fd);
    }
}

// --- OPERAȚII ---

void add_report(const char *dist, const char *user, const char *rol) {
    mkdir(dist, 0750); chmod(dist, 0750);
    char cale[256]; sprintf(cale, "%s/reports.dat", dist);

    Raport r;
    r.id = rand() % 10000;
    strncpy(r.inspector, user, MAX_STR);
    r.data = time(NULL);

    printf("Categorie: "); scanf("%s", r.categorie);
    printf("Gravitate (1-3): "); scanf("%d", &r.gravitate);
    printf("GPS (lat lon): "); scanf("%lf %lf", &r.lat, &r.lon);
    printf("Descriere: "); scanf(" %[^\n]", r.descriere);

    int fd = open(cale, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if (fd < 0) { perror("Acces refuzat"); return; }
    chmod(cale, 0664);
    write(fd, &r, sizeof(Raport));
    close(fd);
    log_action(dist, rol, user, "ADD_REPORT");
}

void list_reports(const char *dist, const char *rol, const char *user) {
    char cale[256], p_str[11]; sprintf(cale, "%s/reports.dat", dist);
    struct stat st;
    if (stat(cale, &st) != 0) return;

    perm_to_str(st.st_mode, p_str);
    printf("Fișier: %s | Permisiuni: %s | Mărime: %ld | Modificat: %s", cale, p_str, st.st_size, ctime(&st.st_mtime));

    int fd = open(cale, O_RDONLY);
    Raport r;
    while(read(fd, &r, sizeof(Raport)) > 0)
        printf("[%d] %s | Sev: %d | De: %s\n", r.id, r.categorie, r.gravitate, r.inspector);
    close(fd);
    log_action(dist, rol, user, "LIST");
}

void view_report(const char *dist, int id) {
    char cale[256]; sprintf(cale, "%s/reports.dat", dist);
    int fd = open(cale, O_RDONLY);
    Raport r;
    while(read(fd, &r, sizeof(Raport)) > 0) {
        if(r.id == id) {
            printf("\nID: %d\nCat: %s\nSev: %d\nGPS: %f, %f\nDesc: %s\n", r.id, r.categorie, r.gravitate, r.lat, r.lon, r.descriere);
            close(fd); return;
        }
    }
    printf("Raport negăsit.\n"); close(fd);
}

void remove_report(const char *dist, int id, const char *rol, const char *user) {
    if (strcmp(rol, "manager") != 0) { printf("Doar managerul poate șterge!\n"); return; }
    char cale[256]; sprintf(cale, "%s/reports.dat", dist);

    int fd = open(cale, O_RDWR);
    if (fd < 0) return;

    Raport r; off_t scriere = 0; int gasit = 0;
    while(read(fd, &r, sizeof(Raport)) > 0) {
        if (r.id == id) { gasit = 1; continue; }
        if (gasit) {
            off_t temp = lseek(fd, 0, SEEK_CUR);
            lseek(fd, scriere, SEEK_SET);
            write(fd, &r, sizeof(Raport));
            lseek(fd, temp, SEEK_SET);
        }
        scriere += sizeof(Raport);
    }
    if (gasit) ftruncate(fd, scriere);
    close(fd);
    log_action(dist, rol, user, "REMOVE_REPORT");
}

void update_threshold(const char *dist, int val, const char *rol, const char *user) {
    if (strcmp(rol, "manager") != 0) return;
    char cale[256]; sprintf(cale, "%s/district.cfg", dist);

    struct stat st;
    if (stat(cale, &st) == 0 && (st.st_mode & 0777) != 0640) {
        printf("Eroare: Permisiunile district.cfg au fost alterate!\n");
        return;
    }

    int fd = open(cale, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd >= 0) {
        dprintf(fd, "threshold=%d\n", val);
        chmod(cale, 0640);
        close(fd);
    }
    log_action(dist, rol, user, "UPDATE_THRESHOLD");
}

// --- MAIN ---

int main(int argc, char *argv[]) {
    char *rol = "inspector", *user = "anonim", *dist = NULL, *cmd = NULL;
    int val = -1;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--role")) rol = argv[++i];
        else if (!strcmp(argv[i], "--user")) user = argv[++i];
        else if (!dist) dist = argv[i];
        else if (!cmd) cmd = argv[i];
        else val = atoi(argv[i]);
    }

    if (!dist || !cmd) return 0;

    if (!strcmp(cmd, "add")) add_report(dist, user, rol);
    else if (!strcmp(cmd, "list")) list_reports(dist, rol, user);
    else if (!strcmp(cmd, "view")) view_report(dist, val);
    else if (!strcmp(cmd, "remove_report")) remove_report(dist, val, rol, user);
    else if (!strcmp(cmd, "update_threshold")) update_threshold(dist, val, rol, user);

    return 0;
}
