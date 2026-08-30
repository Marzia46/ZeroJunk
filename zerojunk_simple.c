/* =====================================================
   ZeroJunk - Simple Junk File Cleaner (Windows)
   -----------------------------------------------------
   A beginner-level C project using only:
     - functions
     - do-while loop
     - switch-case
     - basic strings/arrays
     - standard file I/O (fopen, fprintf, fgets, fclose)
     - system() to run Windows commands
   No pointers to structs, no recursion, no Windows API.
   ===================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Name of the log file where cleaning history is saved */
#define LOG_FILE "zerojunk_log.txt"

/* -----------------------------------------------------
   Function: scanJunk
   Purpose : Shows the contents and total size of the
             Windows Temp folder (%TEMP%) using the
             built-in "dir" command.
   ----------------------------------------------------- */
void scanJunk() {
    printf("\nScanning Temp folder (%%TEMP%%)...\n\n");

    /* /s = include subfolders, dir also prints total size at the end */
    system("dir %TEMP% /s");

    printf("\nScan complete. See total file size above.\n");
}

/* -----------------------------------------------------
   Function: cleanJunk
   Purpose : Deletes all files inside the Temp folder
             using the "del" command, then saves the
             cleaning time to the log file.
   ----------------------------------------------------- */
void cleanJunk() {
    printf("\nCleaning Temp folder (%%TEMP%%)...\n");

    /* /q = quiet mode (no confirmation), /s = include subfolders */
    system("del /q /s %TEMP%\\*.* >nul 2>&1");

    printf("Cleaning complete.\n");

    /* Save the timestamp of this cleaning session */
    FILE *fp;
    fp = fopen(LOG_FILE, "a");  /* "a" = append, so old logs are not erased */

    if (fp == NULL) {
        printf("Could not open log file.\n");
        return;
    }

    time_t now;
    time(&now);  /* get current time */

    /* ctime() returns the time as text, ending with '\n' */
    fprintf(fp, "Cleaned on: %s", ctime(&now));

    fclose(fp);
    printf("Session saved to %s\n", LOG_FILE);
}

/* -----------------------------------------------------
   Function: viewLogs
   Purpose : Reads and displays the saved cleaning
             history from the log file, line by line.
   ----------------------------------------------------- */
void viewLogs() {
    FILE *fp;
    char line[200];  /* simple character array to hold one line at a time */

    fp = fopen(LOG_FILE, "r");

    if (fp == NULL) {
        printf("\nNo logs found yet. Run 'Clean' first.\n");
        return;
    }

    printf("\n----- Cleaning History -----\n");

    /* fgets reads one line at a time until the file ends */
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }

    printf("-----------------------------\n");
    fclose(fp);
}

/* -----------------------------------------------------
   Function: main
   Purpose : Displays the menu and calls the correct
             function based on the user's choice.
   ----------------------------------------------------- */
int main() {
    int choice;

    do {
        printf("\n========= ZeroJunk =========\n");
        printf("1. Scan Junk Files\n");
        printf("2. Clean Junk Files\n");
        printf("3. View Cleaning Logs\n");
        printf("4. Exit\n");
        printf("=============================\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                scanJunk();
                break;
            case 2:
                cleanJunk();
                break;
            case 3:
                viewLogs();
                break;
            case 4:
                printf("\nExiting ZeroJunk. Goodbye!\n");
                break;
            default:
                printf("\nInvalid choice. Please enter 1-4.\n");
        }

    } while (choice != 4);

    return 0;
}
