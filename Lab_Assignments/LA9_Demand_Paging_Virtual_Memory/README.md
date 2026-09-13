# LA9 — Virtual Memory: Demand Paging & Swapping Simulation

## 📋 Objective

Simulate a **virtual memory manager** running `n` processes, each performing `m` binary
searches over large integer arrays, under memory pressure:

- 4 KB pages, 2048 pages per process address space
- 16384 total frames (64 MB) of which only 12288 (48 MB) are usable
- 10 essential pages (code/data) per process

When a process page-faults and **no free frame** exists, the kernel swaps that process
out entirely (all its frames are freed) and it restarts its interrupted search after
being swapped back in. The simulator reports page accesses, page faults, swaps, and the
minimum degree of multiprogramming reached.

## 📁 Files

| File | Description |
|------|-------------|
| `demandpaging.c` | The simulator. Maintains per-process page tables (`unsigned short` entries with a valid bit in bit 15 and an 15-bit frame number), a free-frame pool, a ready queue and a swapped-out queue. Runs each process's binary search step-by-step (one array probe per scheduling turn), loading pages on demand; on frame exhaustion the process is swapped out, and completed processes' frames allow swapped-out processes back in. Output goes to `output.txt` (or `verboseoutput.txt` in VERBOSE mode). |
| `gensearch.c` | Generator — writes `search.txt`: `n m` then, per process, an array size (~1–2M elements) and `m` random search keys. |
| `search.txt` | Sample generated input. |
| `Makefile` | Builds normal and `-DVERBOSE` versions; `make db` regenerates `search.txt` (128 processes, 64 searches). |
| `LA9.pdf`, `LA9_sample.zip` | Assignment statement and sample I/O. |

## 🔨 How to Build & Run

```bash
make db      # builds gensearch and regenerates search.txt (128 processes, 64 searches)
make run     # builds demandpaging, runs it, prints output.txt
make vrun    # verbose build & run (writes verboseoutput.txt)
```

Or manually:

```bash
gcc -Wall -o demandpaging demandpaging.c
gcc -Wall -DVERBOSE -o demandpaging-verbose demandpaging.c
./demandpaging
```

## 🧠 OS Concepts

- **Demand paging** — pages loaded only on first access; page-table valid bits
- **Page faults** counting per probe of the binary search
- **Swapping** a whole process out under memory pressure, and swapping it back in later
- **Degree of multiprogramming** tracking
- Ready queue / swapped-out queue scheduling (FIFO)
