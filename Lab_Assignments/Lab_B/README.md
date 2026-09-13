# Lab B — Recursive File Search by Extension (finall)

## 📋 Objective

Write a utility (`finall`) that recursively scans a directory tree and lists every
**regular file** matching a given **extension**, printing and saving:

```
NO  : OWNER    SIZE     NAME
```

where OWNER is the file owner's **login ID** (resolved from `/etc/passwd`), SIZE is in
bytes, and NAME is the full path. This is a filesystem-traversal exercise using the
POSIX directory and stat APIs.

## 📁 Files

| File | Description |
|------|-------------|
| `finall.c` | The utility. Loads all UID→login mappings from `/etc/passwd` into a cache (falling back to `getpwuid()`), then recursively walks the given directory with `opendir`/`readdir`/`lstat`. Matches regular files by extension (suffix after the last `.`), prints each match with owner/size/path, recurses into subdirectories, and writes the same listing to `finall_<extension>.txt`. Prints a final match count. |
| `LAB.pdf` | Assignment problem statement. |

## 🔨 How to Build & Run

```bash
gcc -Wall -o finall finall.c
./finall <directory> <extension>

# Example:
./finall /home/user c       # list all .c files under /home/user
```

Output appears on the console **and** in `finall_<extension>.txt`.

## 🧠 OS Concepts

- POSIX directory API: `opendir()`, `readdir()`, `closedir()`
- File metadata: `lstat()`, `S_ISREG`/`S_ISDIR`, `st_uid`, `st_size`
- User database parsing: `/etc/passwd`, `getpwuid()`
- Recursive filesystem traversal with path building
