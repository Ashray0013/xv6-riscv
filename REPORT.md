# Consolidated Report — Assignment 7
**xv6: Threads, Synchronization, Deadlock, Scheduling**
Student: Shaan Shan | 2402MC08 | MA3104
Environment: xv6 (public) on QEMU; all kernel changes commented in source.

---

## 1. Q1 — Kernel-Supported Threads (clone / join)

### Design
- A thread is a `struct proc` with `isthread = 1` that **shares** the creator's `pgdir` (no `copyuvm`), while `allocproc()` gives it a private kernel stack.
- `clone(fcn, arg, stack)` programs the trapframe (`eip = fcn`, `esp` = top of the user-provided stack page, `arg` + fake return pushed in cdecl layout) so the thread starts at `fcn(arg)`.
- The open-file table is duplicated (`filedup`) so stdio works inside threads.
- `join()` reaps only `isthread` zombies: frees the kernel stack but **never** `freevm()`s the shared pgdir.
- `wait()` skips threads, so it can never free the shared address space.
- `fork()` by a thread yields a normal process (`copyuvm`, `isthread = 0`).
- Key data: `struct proc` field `isthread`; syscalls `clone = 22`, `join = 23`.

### Result
3 threads increment one shared counter; `counter > 0` in the parent proves a single address space. **Without** synchronization the final value is unreliable (lost updates — "races lost updates" — on overlapping runs; a serialized single-CPU run may coincidentally print 300000 with no guarantee).

## 2. Q2 — Counting Semaphores + Synchronization

### Design
- `struct semaphore { int value; struct spinlock lk; int inuse; }` in a fixed kernel array `sems[10]`.
- `sem_wait`: block via `sleep(&sems[id], &lk)` while `value <= 0`, then decrement.
- `sem_signal`: increment + `wakeup()`; the while-recheck makes xv6's wakeup-all safe.
- Syscalls: `sem_init / sem_wait / sem_signal = 24 / 25 / 26`.

### Before / After (same 3 x 100,000-increment workload on Q1 threads)

| Version | Final counter | Meaning |
|---|---|---|
| before (no lock) | unreliable, < 300000 | lost updates (race) |
| after (semaphore) | exactly 300000, every run | mutual exclusion works |

### Producer-Consumer
Bounded buffer of 5 with `EMPTY = 5`, `FULL = 0`, `BUF (mutex) = 1`; 20 items. The log prints every produce/consume with slot number; producer blocks after 5 unconsumed items (no overflow), consumer blocks on an empty buffer (no underflow); FIFO order 1..20, each item exactly once.

## 3. Q3 — Deadlock Detection + Avoidance

`detect_deadlock()`: work-vector scan — finish any process whose `Request <= work` and add its Allocation back; unfinished leftovers = deadlocked set.

| Test | State | Result |
|---|---|---|
| T1 safe | Avail(1,1); Alloc {0,1},{1,0},{0,0}; Req {1,0},{0,1},{1,1} | all finish -> NO DEADLOCK |
| T2 deadlocked | Avail(0,0); Alloc {1,0},{0,1}; Req {0,1},{1,0} | none finish -> DEADLOCK {P0,P1} |
| T3 banker | Avail(1,1); Alloc {1,1},{1,1}; Max {3,3},{2,2} | state SAFE; P1 req (1,1) grantable; P0 req (1,1) -> UNSAFE -> DENIED |

**Real deadlock:** two forked processes — A holds Pipe1's write end and blocks reading Pipe2; B holds Pipe2's write end and blocks reading Pipe1. Modeling these holdings, `detect_deadlock()` returns both (circular wait A -> B -> A); `kill()` + `wait()` then break the real deadlock (`piperead` honors `p->killed`).

## 4. Q4 — Priority Preemptive Scheduling with Aging

### Design
- `struct proc` gains `priority` (1 highest ... 20 lowest, default 10) and `waitticks`.
- `set_priority` / `get_priority` syscalls.
- `scheduler()` always picks the RUNNABLE process with the **lowest priority number**, scanning from a rotating cursor `rr_idx` so equal priorities run round-robin; the dispatched process resets `waitticks`.
- `age_procs()` runs on every timer tick: RUNNABLE processes accumulate `waitticks`; beyond 30 ticks the priority number is decremented (boost) and the counter resets — explicit starvation prevention.

### Observed (priotest, 2-CPU QEMU)

| Process | Priority | FINISHED tick | Notes |
|---|---|---|---|
| proc0 | 1 | ~100-110 | runs immediately |
| proc1 | 5 | ~100-110 | runs immediately |
| proc2 | 10 | ~200 | starts after high-prio finish |
| proc3 | 20 | ~300 | priority stepped 20->19->18 while waiting (aging) |

Completion time vs priority (bar = ticks to finish):

    prio 1  |#####...................  ~100
    prio 5  |#####...................  ~110
    prio 10 |##########..............  ~200
    prio 20 |###############.........  ~300  (completes only because of aging)

Completion order correlates with priority and the lowest-priority process still finishes — aging demonstrably prevents starvation.

## 5. Problems Faced and Resolutions
- **(a)** Threads printed nothing at first: `clone()` had not duplicated the open-file table, so fd 1 was closed inside threads and `printf` failed silently; fixed with a `filedup()` loop in `clone` (as `fork` does).
- **(b)** xv6 `wakeup()` wakes ALL sleepers: a naive "decrement then sleep if < 0" semaphore loses wakeups; used `while (value <= 0) sleep` + recheck instead.
- **(c)** Single-CPU QEMU serialized short critical sections and masked races (counter coincidentally 300000); widened race windows / SMP runs expose lost updates; semaphore protection makes the result deterministic.
- **(d)** The real pipe deadlock cannot unwind by itself; `kill()` was used and xv6's `piperead` returns -1 for killed readers, so children exit cleanly.
- **(e)** Strict priority scheduling can starve low-priority processes; the 30-tick aging rule demonstrably boosts them until they run.

## 6. Conclusion
All four techniques were implemented and verified inside xv6: shared-address-space threads (clone/join); counting semaphores that fix a real race and solve producer-consumer; user-space deadlock detection/avoidance validated on three matrices plus a genuine two-process pipe deadlock; and a priority preemptive scheduler with aging whose completion order follows priority without starvation. Per-question sources, diffs, READMEs and logs are in `q1/`-`q4/`.
