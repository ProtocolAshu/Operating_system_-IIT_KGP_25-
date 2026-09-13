# LA7 — Boating Problem (Pthreads Synchronization)

## 📋 Objective

Simulate a boating session with `m` boats (5–10) and `n` visitors (20–100), all as
**pthreads**. Each visitor sightsees for a random 30–120 minutes, then wants a boat
ride of 15–60 minutes. Boats and visitors must be **matched pairwise** and start the
ride together (barrier), and the session ends when the last visitor completes a ride.

Synchronization uses **counting semaphores implemented from scratch** (mutex +
condition variable), plus mutexes and per-boat barriers.

## 📁 Files

| File | Description |
|------|-------------|
| `boating.c` | The complete simulation. Defines a custom `semaphore` struct with `P()`/`V()` built on `pthread_mutex_t` + `pthread_cond_t`. Boat threads loop: announce availability (`V(rider)`), wait for a visitor (`P(boat)`), then rendezvous on a 2-party per-boat `pthread_barrier_t` and ride. Visitor threads sightsee, announce readiness (`V(boat)`), wait (`P(rider)`), scan the shared boat-availability arrays under a mutex, and join the boat's barrier. An end-of-session barrier (`EOS`) lets main exit when the final ride completes. |
| `LA7.pdf` | Assignment problem statement. |

## 🔨 How to Build & Run

```bash
gcc -Wall -o boating boating.c -lpthread
./boating 6 25     # 6 boats, 25 visitors (boats: 5-10, visitors: 20-100)
```

Time scaling: 1 simulated minute = 100 ms.

## 🧠 OS Concepts

- **Pthreads**: `pthread_create`, `pthread_join`
- Counting semaphores implemented with `pthread_mutex_t` + `pthread_cond_t`
- Pairwise matching / rendezvous with `pthread_barrier_t` (per boat + end of session)
- Mutual exclusion on shared boat state (`BA`, `BC`, `BT` arrays)
- Clean termination detection (last completed visitor ends the session)
