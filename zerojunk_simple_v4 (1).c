/* =====================================================
   ZeroJunk - Simple Junk File Cleaner (Windows)  -  v4
   -----------------------------------------------------
   Build:  gcc -Os -s zerojunk_simple_v4.c -o zerojunk.exe

   How it works:
     1. isTempSafe() checks that the Temp folder is safe.
     2. buildFileList() deletes any OLD list, asks Windows
        ("dir" command) to write the path of every file in
        %TEMP% into a NEW list file, and checks that this
        worked. Nothing is kept in RAM.
     3. walkList() reads that list ONE line at a time.
        It is used for both Scan and Clean.
     4. Clean reuses the list made during its own scan,
        so the folder is listed only once.

   What is new in v4:
     - Stricter Temp folder check (see isTempSafe).
     - The list file is always fresh (see buildFileList).
     - The command given to system() is built from a path
       that has no special characters (see isTempSafe).
   ===================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      /* isalpha() checks for a drive letter */
#include <time.h>
#include <sys/stat.h>   /* _stat64() reads a file's size / type */

#define LOG_FILE  "zerojunk_log.txt"    /* cleaning history */
#define LIST_FILE "zerojunk_list.tmp"   /* list of file paths */

/* Results shared by the functions.
   unsigned long long = a very big whole number
   (a normal int overflows at 2 GB). */
unsigned long long totalBytes;   /* size of files counted / deleted */
int fileCount;                   /* files counted / deleted */
int skipped;                     /* files that could not be deleted */

/* The Temp folder AFTER it passed every safety check.
   Only isTempSafe() writes to it. Other functions only read it.
   520 = 500 letters max + room for the end mark '\0'. */
char safeTemp[520];

/* -----------------------------------------------------
   Function: clearInput
   Purpose : Throws away extra characters the user typed,
             so they do not confuse the next scanf().
   ----------------------------------------------------- */
void clearInput() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* do nothing: we just read and ignore the characters */
    }
}

/* -----------------------------------------------------
   Function: printSize
   Purpose : Prints a size as bytes, KB, MB or GB.
   ----------------------------------------------------- */
void printSize(unsigned long long b) {
    /* 1 KB = 1024 bytes
       1 MB = 1024 x 1024 = 1048576 bytes
       1 GB = 1024 x 1024 x 1024 = 1073741824 bytes */
    if (b < 1024ULL)                    printf("%llu bytes", b);
    else if (b < 1024ULL * 1024)        printf("%.2f KB", b / 1024.0);
    else if (b < 1024ULL * 1024 * 1024) printf("%.2f MB", b / 1048576.0);
    else                                printf("%.2f GB", b / 1073741824.0);
}

/* -----------------------------------------------------
   Function: fileExists
   Purpose : Returns 1 if the file can be opened, else 0.
   ----------------------------------------------------- */
int fileExists(const char *name) {
    FILE *fp = fopen(name, "r");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

/* -----------------------------------------------------
   Function: isTempSafe
   Purpose : Safety check. Decides if the folder text is
             really a Temp folder, BEFORE anything is
             listed or deleted. If it passes, a cleaned
             copy of the path is saved in safeTemp.
             Returns 1 if safe, 0 if not safe.

   The rules, in order (any failure = stop, nothing touched):
     1. The text exists and its length is reasonable.
     2. It is a FULL path that starts like  C:\
     3. It does not contain ".." (no jumping to other folders).
     4. It has none of the characters that the Windows
        command line treats as special (see below).
     5. The LAST folder name is exactly "Temp" or "Tmp".
     6. The folder really exists on the disk.
   ----------------------------------------------------- */
int isTempSafe(char folder[]) {
    struct _stat64 info;   /* box of facts about a file or folder */
    char *lastSlash;       /* points at the last '\' in the path */
    size_t len;

    /* Rule 1: missing, too short or too long -> not safe.
       3 = length of "C:\" (a whole drive), so we never clean a drive root.
       500 = longest path we accept, so it always fits in the
             arrays used below (safeTemp[520] and command[700]). */
    if (folder == NULL || strlen(folder) <= 3 || strlen(folder) > 500) {
        printf("Error: Could not find a safe Temp folder. Nothing was touched.\n");
        return 0;
    }

    /* Rule 2: must be a full path = drive letter, colon, backslash.
       "C:\Users\..." passes.  "temp" or "..\temp" or "\\server\x" fail.
       (unsigned char) is just the safe way to give a letter to isalpha. */
    if (!isalpha((unsigned char)folder[0]) || folder[1] != ':' || folder[2] != '\\') {
        printf("Error: TEMP is not a full path like C:\\...  Nothing was touched.\n");
        return 0;
    }

    /* Rule 3: ".." means "go up one folder", so it could escape from Temp. */
    if (strstr(folder, "..") != NULL) {
        printf("Error: TEMP contains \"..\". Nothing was touched.\n");
        return 0;
    }

    /* Rule 4: refuse characters that the Windows command line treats
       as special. The path is placed inside a command, so a path
       containing these could change what the command does.
           "  closes the quotes      %  expands a variable
           &  |  <  >  chain / redirect commands
           ^  !  escape / expand characters
           *  ?  wildcards             /  wrong kind of slash
       Spaces are NOT in this list: the quotes around the path
       in the command already make spaces safe.
       strpbrk() returns non-NULL if the text has ANY of these. */
    if (strpbrk(folder, "\"%&|<>^!*?/") != NULL) {
        printf("Error: TEMP contains a special character. Nothing was touched.\n");
        return 0;
    }

    /* Make a private copy and remove a trailing '\' if there is one
       ("C:\Windows\Temp\" becomes "C:\Windows\Temp").
       The copy always fits: folder is at most 500 characters. */
    strcpy(safeTemp, folder);
    len = strlen(safeTemp);
    while (len > 3 && safeTemp[len - 1] == '\\') {
        safeTemp[len - 1] = '\0';
        len--;
    }

    /* Rule 5: the last folder name must be exactly Temp or Tmp.
       strrchr finds the LAST '\'. It cannot be NULL because Rule 2
       already proved that the path has a '\' at position 2.
       _stricmp compares text and ignores upper/lower case
       (it is a Windows function, like _strlwr in v3). */
    lastSlash = strrchr(safeTemp, '\\');
    if (_stricmp(lastSlash + 1, "Temp") != 0 && _stricmp(lastSlash + 1, "Tmp") != 0) {
        printf("Error: The last folder of TEMP is not \"Temp\" or \"Tmp\". Nothing was touched.\n");
        return 0;
    }

    /* Rule 6: the folder must really exist and really be a folder. */
    if (_stat64(safeTemp, &info) != 0 || !(info.st_mode & _S_IFDIR)) {
        printf("Error: The Temp folder does not exist. Nothing was touched.\n");
        return 0;
    }

    return 1;
}

/* -----------------------------------------------------
   Function: buildFileList
   Purpose : After the safety check passes, asks Windows to
             write the full path of every file inside Temp
             into LIST_FILE, one per line.
               /s = include subfolders
               /b = show only the path
               /a-d-l = files only (no folders or links)
               2>nul  = hide error messages

             A fresh list is guaranteed:
               1. the OLD list is deleted first,
               2. the result of the dir command is checked,
               3. the NEW list must exist afterwards.
             Returns 1 if OK, 0 if anything went wrong.
   ----------------------------------------------------- */
int buildFileList() {
    char *tempPath = getenv("TEMP");   /* Windows setting: the Temp folder path */
    char command[700];   /* the dir command: path (max 500) + about 50 fixed
                            characters = about 550, so 700 is enough */
    int code;            /* result of the dir command */

    if (!isTempSafe(tempPath)) {
        return 0;
    }

    /* Step 1: delete any old list, then make sure it is really gone.
       (remove() just reports an error if there was no file: that is fine.) */
    remove(LIST_FILE);
    if (fileExists(LIST_FILE)) {
        printf("Error: Could not delete the old file list. Nothing was touched.\n");
        return 0;
    }

    /* Step 2: run the dir command.
       The path is inside quotes so spaces in it are OK, and Rule 4
       of isTempSafe() already removed characters that could break
       out of the quotes. We use safeTemp (the checked copy). */
    sprintf(command, "dir \"%s\" /s /b /a-d-l > %s 2>nul", safeTemp, LIST_FILE);
    code = system(command);

    /* system() returns the exit code of the command:
         0  = dir worked and found files
         1  = dir found NO files (an empty Temp folder is normal),
              BUT cmd also uses 1 when it cannot create the list file,
              so step 3 below is still needed
         anything else (for example -1) = the command did not run. */
    if (code != 0 && code != 1) {
        printf("Error: The Windows dir command failed. Nothing was touched.\n");
        remove(LIST_FILE);
        return 0;
    }

    /* Step 3: the new list must exist. The old one was deleted in
       step 1, so a list found here can only be the new one. */
    if (!fileExists(LIST_FILE)) {
        printf("Error: The file list was not created. Nothing was touched.\n");
        return 0;
    }

    return 1;
}

/* -----------------------------------------------------
   Function: walkList
   Purpose : Reads LIST_FILE one line (one file) at a time,
             checks the file, and counts it.

   The parameter doDelete is a FLAG: a variable that is
   either 0 or 1 and works like a switch.
       doDelete = 0 -> LOOK ONLY (used by Scan)
                       count files and add up sizes.
       doDelete = 1 -> LOOK AND DELETE (used by Clean)
                       delete each file, then count only
                       the files that were really deleted.
   Reading the list and checking each file are the same
   for both, so one function does both jobs.

   Safety rules inside the loop:
       - a path that is too long for the buffer stops the
         whole run (the list can no longer be trusted),
       - a path that is not inside safeTemp is skipped,
       - a read error while reading the list stops the run.

   Returns 1 if the whole list was read, 0 if the list could
   not be opened or was not read completely.
   ----------------------------------------------------- */
int walkList(int doDelete) {
    FILE *fp = fopen(LIST_FILE, "r");
    char path[2048];       /* holds ONE path. Normal Windows paths are
                              under 260 characters; 2048 is a big safety
                              margin. */
    struct _stat64 info;   /* box that holds facts about one file (we use its size) */
    size_t tempLen = strlen(safeTemp);   /* length of the checked Temp path */

    totalBytes = 0;
    fileCount = 0;
    skipped = 0;

    if (fp == NULL) {
        printf("Error: Could not read the file list.\n");
        return 0;
    }

    while (fgets(path, sizeof(path), fp) != NULL) {

        /* A normal line ends with '\n'. If there is no '\n' and the
           file has not ended, the line was too long for the buffer and
           we only hold half of it. Stop instead of guessing. */
        if (strchr(path, '\n') == NULL && !feof(fp)) {
            printf("Error: A path in the list is too long. Stopped to stay safe.\n");
            fclose(fp);
            return 0;
        }

        path[strcspn(path, "\r\n")] = '\0';   /* remove end-of-line characters */

        /* skip empty lines and ZeroJunk's own files */
        if (path[0] == '\0' || strstr(path, LIST_FILE) || strstr(path, LOG_FILE)) {
            continue;
        }

        /* Extra safety: the path must start with the checked Temp folder
           followed by '\'. Anything else is never touched.
           _strnicmp compares only the first tempLen letters, ignoring case. */
        if (_strnicmp(path, safeTemp, tempLen) != 0 || path[tempLen] != '\\') {
            skipped++;
            continue;
        }

        /* Check the file again right now. If it is missing or
           cannot be read, skip it. The size is read BEFORE
           deleting (it is gone afterwards). */
        if (_stat64(path, &info) != 0) {
            skipped++;
            continue;
        }

        /* remove() returns 0 only if the file was really deleted.
           If it fails (file is locked or protected), skip it. */
        if (doDelete && remove(path) != 0) {
            skipped++;
            continue;
        }

        fileCount++;
        totalBytes += (unsigned long long)info.st_size;
    }

    /* fgets also returns NULL when reading FAILS, not only at the end.
       ferror() tells the two cases apart. */
    if (ferror(fp)) {
        printf("Error: The file list could not be read completely.\n");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

/* -----------------------------------------------------
   Function: scanJunk
   Purpose : Menu 1. Shows how many files are in Temp
             and their total size. Deletes nothing.
   ----------------------------------------------------- */
void scanJunk() {
    printf("\nScanning Temp folder...\n");

    if (buildFileList() && walkList(0)) {   /* 0 = look only */
        printf("\nFiles found : %d\nTotal size  : ", fileCount);
        printSize(totalBytes);
        printf("\n\nScan complete.\n");
    }
    remove(LIST_FILE);   /* we are finished with the list */
}

/* -----------------------------------------------------
   Function: cleanJunk
   Purpose : Menu 2. Scans, asks for confirmation, deletes
             the files, shows the real results and saves
             the cleaning time to the log file.
   ----------------------------------------------------- */
void cleanJunk() {
    FILE *fp;
    char answer = 'n';   /* safe default if scanf fails */
    time_t now;

    printf("\nPreparing to clean Temp folder...\n");

    /* Step 1: scan first (0 = look only) */
    if (!buildFileList() || !walkList(0)) {
        remove(LIST_FILE);
        printf("Cleaning stopped.\n");
        return;
    }

    if (fileCount == 0) {
        remove(LIST_FILE);
        printf("Nothing to clean.\n");
        return;
    }

    /* Step 2: ask for confirmation */
    printf("Found %d files (", fileCount);
    printSize(totalBytes);
    printf(").\nDelete them? (y/n): ");
    scanf(" %c", &answer);
    clearInput();

    if (answer != 'y' && answer != 'Y') {
        remove(LIST_FILE);
        printf("Cancelled. Nothing was deleted.\n");
        return;
    }

    /* Step 3: delete using the SAME list (1 = look and delete) */
    if (!walkList(1)) {
        printf("Warning: The list was not fully processed. The results below may be incomplete.\n");
    }
    remove(LIST_FILE);

    /* Step 4: show the real results */
    printf("\nCleaning complete.\nFiles deleted : %d\n", fileCount);
    printf("Files skipped : %d (in use or protected)\nSpace freed   : ", skipped);
    printSize(totalBytes);
    printf("\n");

    /* Save the time of this cleaning session */
    fp = fopen(LOG_FILE, "a");   /* "a" = append, so old logs are not erased */
    if (fp == NULL) {
        printf("Could not open log file.\n");
        return;
    }
    time(&now);                                    /* get current time */
    fprintf(fp, "Cleaned on: %s", ctime(&now));    /* ctime() text ends with '\n' */
    fclose(fp);
    printf("Session saved to %s\n", LOG_FILE);
}

/* -----------------------------------------------------
   Function: viewLogs
   Purpose : Menu 3. Shows the saved cleaning history.
   ----------------------------------------------------- */
void viewLogs() {
    FILE *fp = fopen(LOG_FILE, "r");
    char line[200];   /* one log line is short (about 35 characters) */

    if (fp == NULL) {
        printf("\nNo logs found yet. Run 'Clean' first.\n");
        return;
    }

    printf("\n----- Cleaning History -----\n");
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }
    printf("-----------------------------\n");
    fclose(fp);
}

/* -----------------------------------------------------
   Function: main
   Purpose : Shows the menu and calls the right function.
   ----------------------------------------------------- */
int main() {
    int choice;
    int result;   /* 1 if scanf read a number */

    do {
        printf("\n========= ZeroJunk =========\n");
        printf("1. Scan Junk Files\n");
        printf("2. Clean Junk Files\n");
        printf("3. View Cleaning Logs\n");
        printf("4. Exit\n");
        printf("=============================\n");
        printf("Enter your choice: ");

        result = scanf("%d", &choice);

        if (result == EOF) {
            choice = 4;          /* input closed -> 4 = Exit safely */
        } else {
            clearInput();
            if (result != 1) {
                choice = 0;      /* not a number -> 0 = invalid choice */
            }
        }

        switch (choice) {
            case 1: scanJunk();  break;
            case 2: cleanJunk(); break;
            case 3: viewLogs();  break;
            case 4: printf("\nExiting ZeroJunk. Goodbye!\n"); break;
            default: printf("\nInvalid choice. Please enter 1-4.\n");
        }
    } while (choice != 4);

    return 0;
}
