# LA3 — CPU Scheduling Simulation (FCFS & Round Robin)

## 📋 Objective

Build an event-driven simulation of CPU scheduling that reads a process trace
(CPU/IO burst sequences with arrival times) and simulates:

1. **FCFS** (First-Come-First-Served)
2. **Round Robin** with quantum = 10
3. **Round Robin** with quantum = 5

For every process it reports turnaround and wait times, and finally average wait time,
total turnaround, CPU idle time, and CPU utilization.

## 📁 Files

| File | Description |
|------|-------------|
| `genproc.c` | Generator — writes `proc.txt` (default 100 processes): each process has an arrival time followed by alternating CPU/IO bursts ending with `-1`. 90% of processes are IO-bound (short CPU bursts), 10% CPU-bound (long bursts). |
| `schedule.c` | The simulator. Reads `proc.txt`, builds PCBs, and runs an event-driven simulation using a min-heap event queue (ARRIVAL, CPU_COMPLETION, IO_COMPLETION, CPU_TIMEOUT) and a FIFO ready queue. Prompts at runtime for the algorithm: `1` = FCFS, `2` = RR q=10, `3` = RR q=5. |
| `input.txt`, `output.txt`, `verbose_output.txt` | Sample input / captured output. |
| `LA3.pdf` | Assignment problem statement. |

## 🔨 How to Build & Run

```bash
gcc -o genproc genproc.c
gcc -o schedule schedule.c

./genproc 100      # generate proc.txt (optional: number of processes)
./schedule         # then enter 1, 2, or 3 at the prompt
```

Verbose mode (per-event trace):

```bash
gcc -DVERBOSE -o schedule schedule.c
./schedule
```

## 🧠 OS Concepts

- Event-driven simulation with a **min-heap** event queue
- PCB (Process Control Block) representation, process states (NEW/READY/RUNNING/WAITING/TERMINATED)
- **FCFS** and **Round Robin** scheduling with preemption on quantum expiry
- Turnaround time, waiting time, CPU idle time, CPU utilization metrics
