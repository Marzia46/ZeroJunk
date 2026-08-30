/* ZeroJunk - simple junk file cleaner
 * Pure Windows build (GCC / MinGW). No POSIX headers or #ifdef branches.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <windows.h>
#include <direct.h>   /* _rmdir */
#include <dirent.h>   /* MinGW ships a Windows-compatible dirent.h */

double threshold_mb = 500.0;

/* Returns nonzero if the path is a symlink / junction / reparse point.
 * We skip these during scanning and deletion so we never follow a link
 * outside the target folder or touch something we shouldn't. */
int is_reparse_point(const char *path) {
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return 0;
    return (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

/* adds up the size of every file in a folder, including subfolders */
long long folder_size(const char *path) {
    long long total = 0;
    DIR *dir = opendir(path);
    if (!dir) return 0;

    struct dirent *e;
    struct stat st;
    char full[1024];

    while ((e = readdir(dir))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(full, sizeof(full), "%s\\%s", path, e->d_name);

        if (is_reparse_point(full)) continue;
        if (stat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            total += folder_size(full);
        } else if (S_ISREG(st.st_mode)) {
            total += st.st_size;
        }
    }

    closedir(dir);
    return total;
}

/* deletes everything inside a folder but keeps the folder itself */
void remove_contents(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *e;
    struct stat st;
    char full[1024];

    while ((e = readdir(dir))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(full, sizeof(full), "%s\\%s", path, e->d_name);

        if (is_reparse_point(full)) continue;
        if (stat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            remove_contents(full);
            _rmdir(full);
        } else if (S_ISREG(st.st_mode)) {
            remove(full);
        }
    }

    closedir(dir);
}

#define MAX_FOLDERS 4

/* fills in the Windows junk-folder list and returns how many there are */
int get_folders(char folders[MAX_FOLDERS][512]) {
    int n = 0;
    char temp[512] = {0};
    char localAppData[512] = {0};

    /* %TEMP% - user's temp folder */
    if (GetEnvironmentVariableA("TEMP", temp, sizeof(temp)) > 0) {
        snprintf(folders[n], 512, "%s", temp);
        n++;
    }

    /* %LOCALAPPDATA%\Temp */
    if (GetEnvironmentVariableA("LOCALAPPDATA", localAppData, sizeof(localAppData)) > 0) {
        snprintf(folders[n], 512, "%s\\Temp", localAppData);
        n++;
    }

    /* System-wide Windows Temp folder */
    snprintf(folders[n], 512, "C:\\Windows\\Temp");
    n++;

    /* Windows Prefetch (safe, regenerable cache data) */
    snprintf(folders[n], 512, "C:\\Windows\\Prefetch");
    n++;

    return n;
}

double total_junk_mb(char folders[MAX_FOLDERS][512], int n) {
    long long bytes = 0;
    for (int i = 0; i < n; i++) bytes += folder_size(folders[i]);
    return bytes / (1024.0 * 1024.0);
}

void log_session(double freed_mb) {
    FILE *fp = fopen("zerojunk_log.txt", "a");
    if (!fp) return;

    time_t now = time(NULL);
    char buf[32];
    ctime_s(buf, sizeof(buf), &now);
    buf[strcspn(buf, "\n")] = '\0';  /* strip trailing newline from ctime_s */

    fprintf(fp, "%.24s | %.2f MB freed\n", buf, freed_mb);
    fclose(fp);
}

void scan(void) {
    char folders[MAX_FOLDERS][512];
    int n = get_folders(folders);
    double total = total_junk_mb(folders, n);
    printf("\nTotal junk found: %.2f MB\n", total);
    if (total > threshold_mb) printf("Warning: exceeds threshold of %.2f MB!\n", threshold_mb);
}

void clean(void) {
    char folders[MAX_FOLDERS][512];
    int n = get_folders(folders);
    double freed = total_junk_mb(folders, n);

    for (int i = 0; i < n; i++) remove_contents(folders[i]);
    printf("\nCleaned %.2f MB of junk.\n", freed);

    log_session(freed);
}

void view_logs(void) {
    FILE *fp = fopen("zerojunk_log.txt", "r");
    if (!fp) { printf("\nNo logs yet.\n"); return; }
    char line[256];
    while (fgets(line, sizeof(line), fp)) printf("%s", line);
    fclose(fp);
}

/* clears out any leftover input so scanf doesn't get stuck */
void clear_input(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void set_threshold(void) {
    printf("\nCurrent threshold: %.2f MB\nNew threshold: ", threshold_mb);
    if (scanf("%lf", &threshold_mb) != 1) {
        printf("Invalid input, threshold unchanged.\n");
    }
    clear_input();
}

int main(void) {
    int choice;
    do {
        printf("\n1.Scan 2.Clean 3.Logs 4.Threshold 5.Exit\nChoice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input, please enter a number.\n");
            clear_input();
            continue;
        }
        clear_input();
        switch (choice) {
            case 1: scan(); break;
            case 2: clean(); break;
            case 3: view_logs(); break;
            case 4: set_threshold(); break;
            case 5: printf("Goodbye!\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 5);
    return 0;
}
