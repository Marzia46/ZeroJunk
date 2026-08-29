# ZeroJunk — Lightweight C Junk Cleaner

**ZeroJunk** is a fast, simple CLI tool written in C to scan and safely clean temporary cache files, logs, and trash on your system without any heavy background apps[cite: 1].

---

## About the Project

Third-party cleaner software often runs heavy processes in the background and takes up too much RAM. Basic scripts, on the other hand, delete files blindly without showing how much space they free.

**ZeroJunk** solves this by calculating the exact size of cleanable files in MB before deleting anything and keeping a log history, all while running instantly with zero background RAM usage[cite: 1].

---

##  Features

- **Pre-Cleanup Scan:** Checks folder sizes recursively and shows total recoverable space in MB before deleting[cite: 1].
- **Safe Cleaning:** Deletes junk from cache, trash, and log folders while leaving root directories intact and skipping symlinks[cite: 1].
- **Threshold Warning:** Warns you if temporary junk crosses your specified MB limit[cite: 1].
- **History Logs:** Saves cleanup dates and freed storage metrics into `zerojunk_log.txt`[cite: 1].
- **Lightweight Menu:** Runs on-demand through an interactive CLI with no background services.

---

##  Concepts Used

- **Language:** C (Standard C99)
- **Directory Traversal:** Using `opendir()`, `readdir()`, and `closedir()`[cite: 1]
- **Recursion:** Deep scanning inside nested subdirectories[cite: 1]
- **File Handling:** Session logging with `fopen()`, `fprintf()`, and `fclose()`[cite: 1]
- **Safety Checks:** Inspecting file types with `lstat()` and `sys/stat.h` to skip symlinks[cite: 1]

---

##  Project Structure

```text
ZeroJunk/
├── zerojunk.c          # Core source code containing scanning and cleaning logic
├── zerojunk_log.txt    # Session history log file (auto-generated)
├── ZeroJunk.pdf        # Official project proposal document
└── README.md           # Project documentation and manual

##  How to Run

Compile and run using GCC in your terminal:

```bash
gcc zerojunk.c -o zerojunk
./zerojunk

sample preview
1.Scan 2.Clean 3.Logs 4.Threshold 5.Exit
Choice: 1

Total junk found: 19159.81 MB
Warning: exceeds threshold of 500.00 MB!

License
This project is developed for academic evaluation purposes as part of the Software Development Lab I course curriculum at Pundra University of Science & Technology[
