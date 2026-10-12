# OS Lab Assignment 7 — xv6 Kernel Extensions

**Course:** MA3104 (Operating Systems)  
**Student:** Ashray Khairnar | 2402MC08  
**Platform:** xv6 (public) on QEMU, with kernel extensions  

---

## Overview

This repository contains a complete solution to the four-question OS Lab Assignment 7. Each question builds a classic operating‑system concept directly on top of the xv6 kernel:

| # | Topic | What was built |
|---|-------|----------------|
| Q1 | Threads | `clone()` / `join()` syscalls — kernel‑supported threads sharing one address space |
| Q2 | Synchronization | Counting semaphores (`sem_init`, `sem_wait`, `sem_signal`) fix a real race + producer‑consumer |
| Q3 | Deadlock | User‑space detection (work‑vector algorithm) + Banker’s avoidance + a real pipe deadlock |
| Q4 | Scheduling | Priority‑preemptive scheduler with aging replaces round‑robin |

All kernel modifications are commented, and each question folder contains the relevant source diffs, user test programs, logs, and screenshots.

---

## Repository Layout

```
.
├── q1/                         # Question 1 – Threads
│   ├── threadtest.c            # 3 threads incrementing a shared counter
│   ├── kernel_diffs/           # Modified kernel files (proc.c, proc.h, sysproc.c, …)
│   ├── README.md               # Q1‑specific notes
│   └── log.txt                 # Terminal output (unsynchronized counter)
│
├── q2/                         # Question 2 – Semaphores
│   ├── synctest.c              # Synchronized counter + producer‑consumer
│   ├── kernel_diffs/           # Semaphore implementation + syscalls
│   ├── README.md
│   └── log.txt                 # Counter = 300000, PC log
│
├── q3/                         # Question 3 – Deadlock
│   ├── deadlock.c              # Detection, Banker’s, pipe deadlock simulation
│   ├── kernel_diffs/           # Any kernel support (e.g., pipes already exist)
│   ├── README.md
│   └── log.txt                 # 3 test matrices + pipe circular wait
│
├── q4/                         # Question 4 – Scheduling
│   ├── priotest.c              # 4 processes with priorities 1,5,10,20
│   ├── kernel_diffs/           # proc.c, proc.h, sysproc.c, trap.c changes
│   ├── README.md
│   └── log.txt                 # Completion order + aging trace
│
├── REPORT.md                   # Consolidated 2‑3 page report
└── README.md                   # This file
```

---

## Quick Start (any question)

1. Copy the modified kernel files from the desired `q*/kernel_diffs/` into your xv6 source tree (or apply the provided patch).
2. Build and run:

   ```bash
   make clean
   make qemu-nox
   ```

3. At the xv6 shell, run the corresponding user program:

   ```bash
   $ threadtest      # Q1
   $ synctest        # Q2
   $ deadlock        # Q3
   $ priotest        # Q4
   ```

> **Note:** Each question folder has its own `README.md` with specific build/run instructions and expected output.

---

## Detailed Solution

### Question 1 — Implementing User‑Level Threads

**Design decisions**

- Added two new system calls: `clone(void (*fcn)(void*), void *arg, void *stack)` and `join(void)`.
- Extended `struct proc` with:
  - `int is_thread;` — marks a process as a thread.
  - `struct proc *parent;` — points to the main process (the one that called `clone`).
  - `uint64 stack;` — user stack pointer for the thread.
- `clone` allocates a new `struct proc` using `allocproc()`, copies the caller’s trapframe, sets the user stack to the provided `stack`, and shares the page directory (`pgdir`) with the parent. The new thread starts execution at `fcn(arg)`.
- `join` waits for a specific thread (identified by its pid) to exit using `wait()`, then frees its kernel stack and `proc` structure.
- `exit()` was modified so that if `is_thread` is true, it does **not** free the shared page directory; it only frees the kernel stack and marks the process as `ZOMBIE`.
- `fork()` and `wait()` were adjusted to correctly handle thread processes (e.g., a thread’s parent is the main process, not the shell).

**User program: `threadtest.c`**

Creates 3 threads. Each thread increments a global `counter` 100,000 times. After all threads finish (via `join`), the main thread prints the final counter.

**Expected outcome**

- All threads share the same address space, so the global counter is visible to all.
- Without synchronization, the final value is typically **less than 300,000** due to race conditions.
- Example output:

  ```
  Final counter value: 274513
  ```

**Files modified:** `proc.h`, `proc.c`, `syscall.c`, `syscall.h`, `sysproc.c`, `defs.h`.

---

### Question 2 — Process/Thread Synchronization using Semaphores

**Design decisions**

- Added a `struct semaphore` in the kernel:

  ```c
  struct semaphore {
      int value;
      struct spinlock lk;
      // (wait queue handled via sleep/wakeup on the semaphore address)
  };
  ```

- A fixed array of 10 semaphores (`sem_table[10]`) is protected by a global spinlock.
- Implemented three system calls:
  - `sem_init(int sem_id, int value)`
  - `sem_wait(int sem_id)`
  - `sem_signal(int sem_id)`

  `sem_wait` decrements `value`; if it becomes negative, the calling process sleeps on the semaphore channel. `sem_signal` increments `value` and wakes one waiting process if any.

- Used xv6’s existing `sleep()` / `wakeup()` mechanism for blocking/waking.

**User program: `synctest.c`**

1. **Synchronized counter** — same as Q1 but each increment is wrapped in `sem_wait` / `sem_signal`. The final counter is exactly **300,000** every run.
2. **Producer‑Consumer** — one producer and one consumer share a buffer of size 5. Semaphores `empty`, `full`, and `mutex` ensure no overflow/underflow. The program prints each produced/consumed item.

**Expected outcome**

- Counter = 300,000 (exact).
- Producer‑Consumer runs without buffer errors; log shows correct ordering.

**Files modified:** `proc.h`, `proc.c`, `syscall.c`, `syscall.h`, `sysproc.c`, `defs.h`.

---

### Question 3 — Deadlock Detection

**Design decisions**

- Implemented entirely as a user‑space program `deadlock.c` (xv6 itself has no multi‑resource allocator).
- Static arrays:
  - `Allocation[n][m]` — resources currently held.
  - `Request[n][m]` or `Max[n][m]` — resources still needed.
  - `Available[m]` — currently available resources.
- `detect_deadlock()` uses the classic work‑vector algorithm:
  1. Work = Available.
  2. Find a process whose Request ≤ Work.
  3. If found, simulate it finishing: Work += Allocation, mark as finished.
  4. Repeat. Any unfinished process is deadlocked.
- `is_safe_state()` implements the Banker’s Algorithm for deadlock avoidance.
- **Real deadlock simulation:** Two processes communicating via pipes:
  - Process A holds the write end of Pipe1 and waits to read from Pipe2.
  - Process B holds the write end of Pipe2 and waits to read from Pipe1.
  - The detection function correctly identifies the circular wait.

**Test matrices**

1. **Deadlocked** — detection returns `{P0, P1}`.
2. **Safe** — detection returns no deadlock.
3. **Unsafe after request** — Banker’s algorithm denies the request.

**Expected outcome**

- Program prints the state (Available, Allocation, Need) at each step.
- Correctly identifies deadlock/no‑deadlock for all three matrices.
- Pipe deadlock is detected and reported.

**Files:** `deadlock.c` (no kernel changes required, but we used existing pipes).

---

### Question 4 — Priority‑Based Preemptive CPU Scheduling

**Design decisions**

- Added `int priority;` to `struct proc` (1 = highest, 20 = lowest). Default on creation = 10.
- Implemented two new system calls:
  - `set_priority(int pid, int priority)`
  - `get_priority(int pid)`
- Modified `scheduler()` in `proc.c`:
  - Always selects the `RUNNABLE` process with the **lowest priority number** (highest priority).
  - Ties are broken in round‑robin order.
- **Aging:** Added `int wait_ticks;` to `struct proc`. On every timer tick, for each `RUNNABLE` process, increment `wait_ticks`. If it exceeds 30, decrement its priority number by 1 (i.e., increase priority) and reset `wait_ticks` to 0. This prevents starvation of low‑priority processes.

**User program: `priotest.c`**

- Forks 4 processes with distinct priorities: 1, 5, 10, and 20.
- Each runs a CPU‑bound loop for a fixed number of iterations.
- Prints completion order and, for the lowest‑priority process, logs priority changes due to aging.

**Expected outcome**

- Completion order correlates with priority: highest priority finishes first.
- The priority‑20 process does not starve; it eventually completes thanks to aging.
- Example completion order: P1 (prio 1), P2 (prio 5), P3 (prio 10), P4 (prio 20) — with P4 showing `20 → 19 → 18` over time.

**Files modified:** `proc.h`, `proc.c` (scheduler, allocproc, fork), `trap.c` (for aging tick), `syscall.c`, `syscall.h`, `sysproc.c`, `defs.h`.

---

## Common Submission Requirements

All required items are included in this repository:

- **Modified kernel source files** — available under each `q*/kernel_diffs/` folder. They are clearly commented and include only the necessary changes.
- **User test programs** — `threadtest.c`, `synctest.c`, `deadlock.c`, `priotest.c`.
- **Terminal output screenshots** — stored as images in each question folder (or referenced in the respective `README.md`).
- **Consolidated report** — see [`REPORT.md`](REPORT.md) for a 2‑3 page write‑up covering:
  - Design decisions and key data structures for each part.
  - Problems faced and how they were resolved.
  - Q1‑Q2: before/after comparison of the race condition.
  - Q3: deadlock detection results across all test cases.
  - Q4: scheduling behaviour graph/table (completion time vs. priority).

---

## Problems Faced and Solutions

| Problem | Solution |
|---------|----------|
| Sharing page directory between threads without corrupting memory. | Used `pgdir` pointer from parent; `exit()` checks `is_thread` before freeing. |
| Semaphore race conditions when multiple processes call `sem_wait` simultaneously. | Protected semaphore array with a global spinlock; used `sleep`/`wakeup` on the semaphore address. |
| Deadlock detection algorithm incorrectly marking safe states as deadlocked. | Carefully implemented work‑vector: only processes with `Request ≤ Work` are considered finishable. |
| Scheduler starvation of low‑priority processes. | Added aging: wait‑tick counter, priority boost after 30 ticks. |
| `fork()` and `wait()` misbehaving with threads. | Adjusted `fork` to copy `is_thread` and `parent`; `wait` only waits for children, not threads. |

---

## Conclusion

This assignment provided hands‑on experience with four fundamental OS concepts. The xv6 kernel was successfully extended to support user‑level threads, counting semaphores, deadlock detection/avoidance, and a priority‑based preemptive scheduler with aging. All test programs demonstrate the expected behaviour, and the race condition in Q1 is clearly fixed in Q2. The deadlock detector correctly identifies circular waits, and the scheduler proves that aging prevents starvation.

For any questions, please refer to the individual `README.md` files in each question folder.