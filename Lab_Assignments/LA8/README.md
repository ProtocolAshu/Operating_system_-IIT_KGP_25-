# LA8 — Deadlock Avoidance / Detection with the Banker's Algorithm

## 📋 Objective

Simulate `n` user threads requesting and releasing `m` resource types from a **master
(banker) thread**. Each user thread reads a script of timed requests (`R` = request with
positive/negative components; `Q` = quit). The master thread grants requests only when
resources are available and — when compiled with `-D_DLAVOID` — only when granting keeps
the system in a **safe state** (Banker's algorithm). Otherwise the requesting thread
waits in a pending queue until other threads release resources.

Two modes:

- **Without `-D_DLAVOID`**: grants whenever resources are available (deadlock possible).
- **With `-D_DLAVOID`**: full Banker's-style safety check before every grant.

## 📁 Files

| File | Description |
|------|-------------|
| `resource.c` | The simulator. Main thread runs the master logic; `n` user threads are created via `pthread_create`. Synchronization: a Beginning-Of-Session barrier, a request barrier + per-thread ACK barriers, and per-thread condition variables on which threads wait until their request is granted. The master maintains `AVAILABLE`, `ALLOCATION`, `NEED` matrices, a pending-request linked-list queue, and the safety algorithm (compiled in only with `-D_DLAVOID`). |
| `geninput.c` | Generator — run as `./geninput m n`: writes `input/system.txt` (m, n, TOTAL resources) and `input/thread00.txt … threadNN.txt` (max needs line + a random script of 5–10 timed `R` requests and a final `Q`). Configurable to bias toward deadlock or merely unsafe states (see commented line). |
| `LA8.pdf`, `LA8_Sample.zip` | Assignment statement and sample I/O. |

## 🔨 How to Build & Run

```bash
gcc -o geninput geninput.c
mkdir -p input
./geninput 5 10            # 5 resource types, 10 threads

# Mode 1 — grant if available (deadlocks possible):
gcc -Wall -o resource resource.c -lpthread
./resource

# Mode 2 — deadlock avoidance (Banker's algorithm):
gcc -Wall -D_DLAVOID -o resource resource.c -lpthread
./resource
```

`resource` reads `input/system.txt` and `input/threadNN.txt`.

## 🧠 OS Concepts

- **Banker's algorithm** (deadlock avoidance) — safety check on a tentative allocation
- **Deadlock** behavior when granting greedily (waiting threads never woken)
- Pthreads: barriers, per-thread condition variables, mutexes
- Resource-allocation state: `AVAILABLE`, `ALLOCATION`, `NEED` matrices
- Pending-request queue with retry on every state change
