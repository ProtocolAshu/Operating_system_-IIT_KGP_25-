# LA1 — Recursive Rebuild of Foo-modules (make-like dependency resolution)

## 📋 Objective

Simulate a simplified version of `make`: given a set of **foo-modules** with dependency
relations stored in `foodep.txt`, rebuild a target foo-module by recursively rebuilding
all of its dependencies first (post-order traversal), using separate processes for each
rebuild step.

This assignment demonstrates **process creation** with `fork()` and **program execution**
with `execlp()`, plus inter-process coordination through files.

## 📁 Files

| File | Description |
|------|-------------|
| `gendep.c` | Generator — creates a random dependency file `foodep.txt` for `n` foo-modules (default 10). Produces a random permutation of modules and a random DAG of dependencies. |
| `rebuild.c` | The recursive rebuild program. Invoked as `./rebuild <foodule>` for the root, and `./rebuild <dep> child` for dependencies (via `fork()` + `execlp()`). Tracks which modules are already rebuilt in `done.txt` so shared dependencies are not rebuilt twice. Prints `fooX rebuilt from fooA, fooB, ...` messages. |
| `LA1.pdf` | Assignment problem statement. |

## 🔨 How to Build & Run

```bash
gcc -o gendep gendep.c
gcc -o rebuild rebuild.c

./gendep 10        # generate foodep.txt (optional: pass number of modules)
./rebuild 5        # rebuild foo-module 5 (recursive)
```

- `foodep.txt` format: first line is `n`, then `n` lines of `u: v1 v2 ...` meaning module
  `u` depends on modules `v1, v2, ...`.
- `rebuild` initializes `done.txt` when run without the `child` argument; recursive
  children are spawned with the `child` marker so they don't reset it.

## 🧠 OS Concepts

- `fork()` — creating child processes
- `execlp()` — replacing the child image with a fresh `rebuild` process
- `wait()` — parent waits for each dependency's rebuild to finish
- File-based shared state (`done.txt`) for de-duplicating rebuilds
