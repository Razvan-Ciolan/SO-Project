#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>

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

// --- AI-ASSISTED FUNCTIONS FOR FILTERING ---

/* Splits a string of type "field:operator:value" */
int parse_condition(const char *input, char *field, char *op, char *value) {
    // Using sscanf with %[ ^:] to read until the ':' character is encountered
    if (sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3) {
        return 1;
    }
    return 0;
}

/* Checks if a report 'r' satisfies a specific condition */
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val_comp = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val_comp;
        if (strcmp(op, "!=") == 0) return r->severity != val_comp;
        if (strcmp(op, "<") == 0)  return r->severity < val_comp;
        if (strcmp(op, "<=") == 0) return r->severity <= val_comp;
        if (strcmp(op, ">") == 0)  return r->severity > val_comp;
        if (strcmp(op, ">=") == 0) return r->severity >= val_comp;
    }
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    }
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector, value) != 0;
    }
    else if (strcmp(field, "timestamp") == 0) {
        time_t val_comp = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == val_comp;
        if (strcmp(op, "!=") == 0) return r->timestamp != val_comp;
        if (strcmp(op, "<") == 0)  return r->timestamp < val_comp;
        if (strcmp(op, ">") == 0)  return r->timestamp > val_comp;
    }
    return 0;
}

// PERMISSIONS (Bit-to-Symbol manual conversion)

void perm_to_str(mode_t m, char *s) {
    if (m & S_IRUSR) s[0] = 'r'; else s[0] = '-';
    if (m & S_IWUSR) s[1] = 'w'; else s[1] = '-';
    if (m & S_IXUSR) s[2] = 'x'; else s[2] = '-';
    if (m & S_IRGRP) s[3] = 'r'; else s[3] = '-';
    if (m & S_IWGRP) s[4] = 'w'; else s[4] = '-';
    if (m & S_IXGRP) s[5] = 'x'; else s[5] = '-';
    if (m & S_IROTH) s[6] = 'r'; else s[6] = '-';
    if (m & S_IWOTH) s[7] = 'w'; else s[7] = '-';
    if (m & S_IXOTH) s[8] = 'x'; else s[8] = '-';
    s[9] = '\0';
}

void log_action(const char *dist, const char *role, const char *user, const char *msg) {
    char path[256]; sprintf(path, "%s/logged_district", dist);
    if (access(path, F_OK) == 0 && strcmp(role, "manager") != 0) return;

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        chmod(path, 0644);
        time_t now = time(NULL);
        char *t = ctime(&now); t[strlen(t)-1] = '\0';
        dprintf(fd, "[%s] %s (%s): %s\n", t, user, role, msg);
        close(fd);
    }
}

// SYMLINK MANAGEMENT

void update_symlink(const char *dist) {
    char link_name[256];
    char target_path[256];
    sprintf(link_name, "active_reports-%s", dist);
    sprintf(target_path, "%s/reports.dat", dist);

    // Clean up: remove existing link if it exists to update it
    unlink(link_name);

    // Create new symlink: target -> link_name
    if (symlink(target_path, link_name) == 0) {
        printf("Symlink updated: %s -> %s\n", link_name, target_path);
    }
}

// OPERATIONS

void add_report(const char *dist, const char *user, const char *role) {
    mkdir(dist, 0750); chmod(dist, 0750);
    char path[256]; sprintf(path, "%s/reports.dat", dist);

    Report r;
    r.id = rand() % 10000;
    strncpy(r.inspector, user, MAX_STR);
    r.timestamp = time(NULL);

    printf("Category: "); scanf("%s", r.category);
    printf("Severity (1-3): "); scanf("%d", &r.severity);
    printf("GPS (lat lon): "); scanf("%lf %lf", &r.lat, &r.lon);
    printf("Description: "); scanf(" %[^\n]", r.description);

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if (fd < 0) { perror("Access denied"); return; }
    chmod(path, 0664);
    write(fd, &r, sizeof(Report));
    close(fd);

    update_symlink(dist); // Requirement: Create/Update symlink
    log_action(dist, role, user, "ADD_REPORT");
}

void list_reports(const char *dist, const char *role, const char *user) {
    char path[256], p_str[11]; sprintf(path, "%s/reports.dat", dist);
    struct stat st;
    if (stat(path, &st) != 0) { printf("No reports found.\n"); return; }

    perm_to_str(st.st_mode, p_str);
    printf("File: %s | Permissions: %s | Size: %ld | Modified: %s", path, p_str, (long)st.st_size, ctime(&st.st_mtime));

    int fd = open(path, O_RDONLY);
    Report r;
    while(read(fd, &r, sizeof(Report)) > 0)
        printf("[%d] %s | Sev: %d | By: %s\n", r.id, r.category, r.severity, r.inspector);
    close(fd);
    log_action(dist, role, user, "LIST");
}

void view_report(const char *dist, int id) {
    char path[256]; sprintf(path, "%s/reports.dat", dist);
    int fd = open(path, O_RDONLY);
    Report r;
    while(read(fd, &r, sizeof(Report)) > 0) {
        if(r.id == id) {
            printf("\nID: %d\nCat: %s\nSev: %d\nGPS: %f, %f\nDesc: %s\n", r.id, r.category, r.severity, r.lat, r.lon, r.description);
            close(fd); return;
        }
    }
    printf("Report not found.\n"); close(fd);
}

void remove_report(const char *dist, int id, const char *role, const char *user) {
    if (strcmp(role, "manager") != 0) { printf("Only managers can delete reports!\n"); return; }
    char path[256]; sprintf(path, "%s/reports.dat", dist);

    int fd = open(path, O_RDWR);
    if (fd < 0) return;

    Report r; off_t write_pos = 0; int found = 0;
    while(read(fd, &r, sizeof(Report)) > 0) {
        if (r.id == id) { found = 1; continue; }
        if (found) {
            off_t temp = lseek(fd, 0, SEEK_CUR);
            lseek(fd, write_pos, SEEK_SET);
            write(fd, &r, sizeof(Report));
            lseek(fd, temp, SEEK_SET);
        }
        write_pos += sizeof(Report);
    }
    if (found) ftruncate(fd, write_pos);
    close(fd);
    log_action(dist, role, user, "REMOVE_REPORT");
}

void update_threshold(const char *dist, int val, const char *role, const char *user) {
    if (strcmp(role, "manager") != 0) return;
    char path[256]; sprintf(path, "%s/district.cfg", dist);

    struct stat st;
    if (stat(path, &st) == 0 && (st.st_mode & 0777) != 0640) {
        printf("Error: district.cfg permissions have been altered!\n");
        return;
    }

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd >= 0) {
        dprintf(fd, "threshold=%d\n", val);
        chmod(path, 0640);
        close(fd);
    }
    log_action(dist, role, user, "UPDATE_THRESHOLD");
}

void filter_reports(const char *dist, int argc, char *argv[], int start_idx) {
    char path[256]; sprintf(path, "%s/reports.dat", dist);
    int fd = open(path, O_RDONLY);
    if (fd < 0) { printf("Data file not found.\n"); return; }

    Report r;
    int count = 0;
    while (read(fd, &r, sizeof(Report)) > 0) {
        int matches_all = 1;
        for (int i = start_idx; i < argc; i++) {
            char field[50], op[10], value[MAX_STR];
            if (parse_condition(argv[i], field, op, value)) {
                if (!match_condition(&r, field, op, value)) {
                    matches_all = 0;
                    break;
                }
            }
        }
        if (matches_all) {
            printf("[%d] %s | Sev: %d | Insp: %s | Desc: %s\n",
                   r.id, r.category, r.severity, r.inspector, r.description);
            count++;
        }
    }
    if (count == 0) printf("No reports matching the criteria.\n");
    close(fd);
}

void scan_links() {
    struct dirent *de;
    DIR *dr = opendir(".");
    if (dr == NULL) return;

    printf("\n--- Scanning for Symlinks (using lstat) ---\n");
    while ((de = readdir(dr)) != NULL) {
        struct stat linfo, sinfo;

        // Use lstat to identify the link itself
        if (lstat(de->d_name, &linfo) == 0) {
            if (S_ISLNK(linfo.st_mode)) {
                printf("Found Link: %s", de->d_name);

                // Use stat to check if the destination exists (detect dangling)
                if (stat(de->d_name, &sinfo) != 0) {
                    printf(" -> WARNING: DANGLING LINK!\n");
                } else {
                    printf(" -> OK (Target size: %ld bytes)\n", (long)sinfo.st_size);
                }
            }
        }
    }
    closedir(dr);
}

// MAIN

int main(int argc, char *argv[]) {
    char *role = "inspector", *user = "anonymous", *dist = NULL, *cmd = NULL;
    int val = -1;
    int cmd_pos = -1;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--role") && i + 1 < argc) {
            role = argv[++i];
        } else if (!strcmp(argv[i], "--user") && i + 1 < argc) {
            user = argv[++i];
        } else if (!dist && argv[i][0] != '-') {
            dist = argv[i];
        } else if (!cmd && argv[i][0] != '-') {
            cmd = argv[i];
            cmd_pos = i;
        } else if (val == -1 && argv[i][0] != '-') {
            val = atoi(argv[i]);
        }
    }

    if (dist && !strcmp(dist, "scan")) {
        scan_links();
        return 0;
    }

    if (!dist || !cmd) {
        printf("Usage:\n");
        printf("  %s <district> <command> [args]\n", argv[0]);
        printf("  %s scan (to check all symlinks)\n", argv[0]);
        return 0;
    }

    if (!strcmp(cmd, "add")) {
        add_report(dist, user, role);
    } else if (!strcmp(cmd, "list")) {
        list_reports(dist, role, user);
    } else if (!strcmp(cmd, "view")) {
        view_report(dist, val);
    } else if (!strcmp(cmd, "remove_report")) {
        remove_report(dist, val, role, user);
    } else if (!strcmp(cmd, "update_threshold")) {
        update_threshold(dist, val, role, user);
    } else if (!strcmp(cmd, "filter")) {
        filter_reports(dist, argc, argv, cmd_pos + 1);
    } else if (!strcmp(cmd, "scan")) {
        scan_links();
    }

    return 0;
}
