# LA4 — Foodoku (Distributed Sudoku with Pipes)

## 📋 Objective

Implement a distributed **Sudoku**-style game ("Foodoku"): the 9×9 board is split into
nine 3×3 blocks, each managed by a **separate process** running in its own `xterm`
window. The coordinator generates a puzzle and dispatches board state to the block
processes over **pipes**. When a player places a digit, the block process checks its own
3×3 block locally and queries its **row and column neighbor blocks** over pipes to
detect conflicts — a decentralized constraint check.

## 📁 Files

| File | Description |
|------|-------------|
| `coordinator.c` | Creates 9 pipes, forks 9 children (each `execlp`-ing an `xterm` running `./block`), then serves a command menu: `n` (new game), `p b c d` (place digit `d` in cell `c` of block `b`), `s` (show solution), `q` (quit). Sends board state and commands through the pipes. |
| `boardgen.c` | Generates a random Sudoku puzzle/solution pair (`A` = puzzle, `S` = solution) from a seeded base board by shuffling rows/columns within bands, permuting digits, and optionally transposing. `#include`-d by the coordinator. |
| `block.c` | The per-block process. `dup2()`s its pipe read-end onto stdin, draws its 3×3 block, and handles commands: `n` (new board), `p` (place digit — checks block conflict locally, then sends `r`/`c` queries to its 2 row-neighbors and 2 column-neighbors and reads their responses), `r`/`c` (answer a neighbor's conflict query), `q` (exit). |
| `Makefile` | Builds `block` and `coordinator`. |
| `LA4.pdf` | Assignment problem statement. |

## 🔨 How to Build & Run

Requires **Linux** with `xterm` installed.

```bash
make          # builds block and coordinator
./coordinator # or: make run
```

Nine xterm windows open (one per block, laid out in a 3×3 grid). Commands in the
coordinator terminal:

```
h        help
n        new game
p b c d  place digit d in cell c of block b   (blocks/cells numbered 0-8)
s        show solution
q        quit
```

## 🧠 OS Concepts

- `pipe()` — 9 coordinator↔block channels plus block↔block conflict queries
- `fork()` + `execlp()` — launching the block processes inside `xterm`
- `dup2()` — redirecting a pipe end to stdin
- Closing inherited pipe ends in children to get correct EOF semantics
- Decentralized conflict detection (each block answers only for its own row/column)
