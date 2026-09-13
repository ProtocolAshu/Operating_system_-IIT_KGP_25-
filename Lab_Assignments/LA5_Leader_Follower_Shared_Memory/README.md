# LA5 — Leader–Follower Turn-Taking with Shared Memory

## 📋 Objective

Implement a **leader–follower** system synchronized purely through **shared memory**:
a leader process and `n` follower processes share an integer array `M`. The leader
generates a random number, then followers take turns (round-robin, via a turn variable
in shared memory) each adding their own random number. The leader prints the sum.
This repeats until a sum **repeats**, at which point everyone terminates gracefully.

No semaphores or signals — synchronization is by **busy waiting on a shared turn
variable**.

## 📁 Files

| File | Description |
|------|-------------|
| `leader.cpp` | Creates the shared-memory segment (`shmget` with `IPC_CREAT \| IPC_EXCL`) using `ftok("/", 65)`, waits for all `n` followers to join, then loops: writes a random number to `M[3]`, passes the turn, waits for the round to complete, computes and prints the sum, and checks a hash table of past sums for a repeat. On repeat, sets the termination turn (`-1` chain) and cleans up the segment (`IPC_RMID`). |
| `follower.cpp` | A manager process that attaches to the existing shared memory, forks the requested number of follower children, and waits for them. Each follower takes its turn when `M[2]` matches its number, writes a random digit to `M[3+i]`, and passes the turn to the next follower (or back to the leader). A negative turn value signals termination; followers exit in a chain printing "follower i leaves". |
| `LA5.pdf` | Assignment problem statement. |

## 🔨 How to Build & Run

Two terminals (or run the follower in the background):

```bash
g++ -o leader leader.cpp
g++ -o follower follower.cpp

# Terminal 1:
./leader 10        # leader expecting 10 followers
# Terminal 2:
./follower 10      # spawn 10 followers (run AFTER the leader)
```

Run order matters: the leader must create the shared memory first; the follower
errors with "Leader is not running" otherwise. At most one leader can run at a time
(`IPC_EXCL`).

## 🧠 OS Concepts

- System V **shared memory**: `ftok()`, `shmget()`, `shmat()`, `shmdt()`, `shmctl(IPC_RMID)`
- Busy-wait synchronization via a shared **turn variable**
- `fork()` for creating followers
- Graceful distributed termination via a signaling chain (negative turn values)
- Duplicate-sum detection with a hash table
