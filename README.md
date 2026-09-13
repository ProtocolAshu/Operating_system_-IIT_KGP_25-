# Operating Systems — IIT Kharagpur (2025)

Lab assignments, lab slides, and theory lecture slides for the Operating Systems course
at IIT Kharagpur. All lab assignments are implemented in C/C++ on Linux, using POSIX
system calls (`fork`, `exec`, signals, pipes, System V IPC, pthreads).

> Author: **Animesh Kumar** (22CS30009) — [ProtocolAshu](https://github.com/ProtocolAshu)

## 📁 Repository Structure

```
Operating_system_-IIT_KGP_25-/
├── Lab_Assignments/     # 10 solved lab assignments (C/C++)
│   ├── LA1_…LA9_, Lab_B_ # each with its own README.md
├── Lab_slides/          # Lab session slides & sample code
└── Theory_slides/       # Theory lecture slides + tutorials
```

## 🧪 Lab Assignments

| # | Assignment | Topic | Key Concepts |
|---|------------|-------|--------------|
| [LA1](Lab_Assignments/LA1_Dependency_Rebuild) | Recursive rebuild of foo-modules | make-like dependency resolution | `fork()`, `execlp()`, `wait()` |
| [LA2](Lab_Assignments/LA2_Catch_Miss_Ball_Game) | Catch–Miss ball game | signal-driven game simulation | `sigaction()`, `kill()`, SIGUSR1/SIGUSR2/SIGINT |
| [LA3](Lab_Assignments/LA3_CPU_Scheduling_Simulator) | CPU scheduling simulator | FCFS & Round Robin (q=10, q=5) | event-driven simulation, min-heap, PCB |
| [LA4](Lab_Assignments/LA4_Foodoku_Distributed_Sudoku) | Foodoku (distributed Sudoku) | 9 block processes in xterm windows | `pipe()`, `dup2()`, block↔block messaging |
| [LA5](Lab_Assignments/LA5_Leader_Follower_Shared_Memory) | Leader–Follower | turn-taking via shared memory | System V `shmget`/`shmat`, busy-wait sync |
| [LA6](Lab_Assignments/LA6_Restaurant_Simulation) | Restaurant simulation | cooks, waiters, customers (11am–3pm) | System V semaphores, shared memory, producer–consumer |
| [LA7](Lab_Assignments/LA7_Boating_Pthreads) | Boating problem | pairwise boat–visitor matching | pthreads, custom semaphores, barriers |
| [LA8](Lab_Assignments/LA8_Bankers_Algorithm_Deadlock) | Deadlock avoidance | Banker's algorithm | safety check, resource matrices, condvars |
| [LA9](Lab_Assignments/LA9_Demand_Paging_Virtual_Memory) | Demand paging & swapping | binary searches under memory pressure | page tables, page faults, swap in/out |
| [Lab B](Lab_Assignments/Lab_B_Recursive_File_Search) | File search utility (`finall`) | recursive search by extension | `opendir`/`readdir`/`lstat`, `/etc/passwd` |

Each assignment folder contains its own `README.md` with the objective, file
descriptions, and build/run instructions.

## 🚀 Getting Started

Assignments are plain C/C++ and build with `gcc`/`g++` on Linux. Some use `make`:

```bash
cd Lab_Assignments/LA2_Catch_Miss_Ball_Game
make && make run
```

Requirements: any Linux distribution (or WSL), `gcc`, `make`, and `xterm` (LA4 only).

## 📚 Course Material

- **Lab slides**: processes, pipes, signals, semaphores, shared memory, threads,
  file system
- **Theory slides**: intro, processes, memory management, file systems, disk I/O
- **Tutorials**: deadlock, semaphores

## 🧠 Topics Covered

- Process Management (`fork`/`exec`/`wait`)
- Signals and signal handling
- CPU Scheduling (FCFS, Round Robin)
- Inter-Process Communication (pipes, shared memory, semaphores)
- Threads & Synchronization (pthreads, mutexes, condition variables, barriers)
- Deadlock (Banker's algorithm)
- Virtual Memory (demand paging, swapping)
- File Systems (POSIX directory/stat APIs)
