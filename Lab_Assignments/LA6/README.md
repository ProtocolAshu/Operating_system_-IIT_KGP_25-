# LA6 — Restaurant Simulation (Semaphores & Shared Memory)

## 📋 Objective

Simulate a restaurant operating from **11:00 am to 3:00 pm** with:

- 10 tables
- 5 waiters (round-robin assignment)
- 2 cooks (5 minutes cooking time per person in an order)

Customers arrive per a schedule file, occupy a table, place an order (1–4 persons),
wait for the food, eat for 30 minutes, and leave. Customers arriving after 3:00 pm or
when no table is free simply leave. All entities run as **separate processes**
synchronized with **System V semaphores** and a large **shared-memory** array `M`.

## 📁 Files

| File | Description |
|------|-------------|
| `cook.c` | The cook process (2 of them, forked from one binary). Creates the shared memory and all semaphores (`ftok("cook.c", 'R')`). Waits on `sem_cook` for orders in the cook queue, cooks (5 min/person), sets the food-ready slot `FR` for the waiter, and signals the waiter. Exits at 3:00 pm when no orders remain. |
| `waiter.c` | The waiter process (5 of them). Each waiter has a 200-slot queue in shared memory. Waits on its semaphore; serves food first (`FR` set, signals the customer's semaphore), otherwise takes the next order (1 minute), pushes it to the cook queue, and signals a cook. |
| `customer.c` | The main customer driver — reads `customers.txt`, forks one process per customer (with inter-arrival sleeps), assigns tables/waiters round-robin, and waits for all children. It also removes the IPC objects at the end. Each customer waits on its own dedicated semaphore (out of 200) until served, eats 30 minutes, frees the table, and leaves. |
| `gencustomers.c` | Generator — writes a random customer schedule to stdout (`id arrival_time count`, terminated by `-1`): 7 customers at opening time, then arrivals until just past 250 minutes. |
| `customers.txt` | Sample generated customer schedule. |
| `Makefile` | Builds `cook`, `waiter`, `customer`; `make db` regenerates `customers.txt`. |
| `LA6.pdf`, `LA6_sample.zip` | Assignment statement and sample I/O. |

## 🔨 How to Build & Run

```bash
make              # builds cook, waiter, customer
make db           # (optional) regenerate customers.txt
./cook            # start the 2 cooks (creates IPC objects)
./waiter          # start the 5 waiters (in another terminal)
./customer        # start the customers (in a third terminal)
```

Run order: `cook` first (it creates the shared memory and semaphores), then `waiter`,
then `customer`. One simulated minute = 100 ms.

## 🧠 OS Concepts

- System V **semaphores** (`semget`/`semop`/`semctl`): a mutex, a cook semaphore, 5
  waiter semaphores, and a 200-unit customer semaphore set
- System V **shared memory** for queues, time, table count, and counters
- Producer–consumer queues (customer → waiter → cook → waiter → customer)
- Multiprocess simulation with a shared logical clock (time only moves forward)
