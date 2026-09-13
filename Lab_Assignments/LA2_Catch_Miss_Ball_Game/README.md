# LA2 — Catch-Miss Ball Game (Signals & Synchronization)

## 📋 Objective

Simulate a game of catch among `n` children: the parent tosses a virtual ball to a child,
who either **catches** it (probability ~80%) or **misses**. A child who misses is out.
The game continues until only one child remains — the winner.

All communication between the parent and the children happens **purely via signals**
(`SIGUSR1`, `SIGUSR2`, `SIGINT`), with file-based PID exchange.

## 📁 Files

| File | Description |
|------|-------------|
| `parent.c` | Creates `n` children, writes their PIDs to `childpid.txt`, then runs the game loop: forks a `dummy` process per round (used as a synchronization barrier via its death), sends the ball with `SIGUSR2`, and tracks catch/miss from the child's reply signal. Sends `SIGINT` to all children at game end. |
| `child.c` | Each child reads all PIDs from `childpid.txt`, then waits on signals. On `SIGUSR2` (ball): catches with 80% probability, replying `SIGUSR1` to the parent, or misses replying `SIGUSR2`. On `SIGUSR1`: prints its CATCH/MISS status cell and forwards the signal down the chain; the last child kills the dummy process to signal end-of-round. On `SIGINT`: declares the winner and exits. |
| `dummy.c` | A trivial process that only calls `pause()` in a loop; it exists to be killed at the end of each status-printing round, acting as a synchronization point the parent `waitpid()`s on. |
| `Makefile` | Builds `parent`, `child`, `dummy`. |
| `LA2.pdf` | Assignment problem statement. |

## 🔨 How to Build & Run

```bash
make          # builds parent, child, dummy
make run      # runs ./parent 8
# or manually:
./parent 8    # play with 8 children
```

## 🧠 OS Concepts

- `fork()` + `execl()` — creating the children
- `sigaction()` — installing signal handlers
- `kill()` — sending signals between processes
- `waitpid()` — reaping the dummy barrier process
- File-based PID exchange (`childpid.txt`, `dummycpid.txt`)
- Signal-driven coordination without shared memory
