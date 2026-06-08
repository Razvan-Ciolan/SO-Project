# City Hub: UNIX Process Management

This is a multi-process command-line application written in C. I built this project to simulate a distributed city management tool and to get hands-on experience with Linux System Programming.

### How it works

The system is divided into four interconnected programs. The Hub acts as the central interactive shell that orchestrates the environment. From there, it can dynamically spawn the Scorer to calculate data in the background and send the results back to the main screen using anonymous pipes.

For data handling, there is a Manager utility that directly manipulates binary files. The interesting part is the Monitor, a daemon that runs quietly in the background. Whenever the Manager updates the database, it triggers a POSIX signal to wake up the Monitor and log the activity in real-time.

### Under the hood

Instead of relying on high-level libraries, this project interacts directly with the Linux kernel using standard POSIX system calls. 

Some of the core technical implementations include:
* Process management and synchronization using fork, exec, and waitpid to ensure no zombie processes are left behind.
* Inter-Process Communication (IPC) via pipes and standard output redirection using dup2.
* Custom event triggers and graceful shutdowns using signals like SIGUSR1 and SIGINT.
* Mutual exclusion concurrency control implemented through a PID file lock.
* Low-level file system auditing to detect broken symbolic links using stat and lstat, alongside in-place binary editing using lseek.

### Running the project

To compile the entire suite manually, run the following commands in your terminal:

```bash
gcc city_hub.c -o city_hub
gcc city_manager.c -o city_manager
gcc monitor_reports.c -o monitor_reports
gcc scorer.c -o scorer
```
To test the inter-process communication, open two terminals. In the first terminal, start the main hub and initialize the background monitor:

```bash
./city_hub
hub_shell> start_monitor
```
In the second terminal, use the manager to add a new report. You will see the monitor catch the signal and print the alert in the first terminal:

```bash
./city_manager centru add
```
You can also test the file system auditing tool to scan for dangling symlinks by running:
```bash
./city_manager scan
```
