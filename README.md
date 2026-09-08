# Shared Memory and Peterson's Algorithm in xv6

This project adds a minimal shared-memory mechanism to xv6 and then uses it to implement a two-process Peterson lock for mutual exclusion.

The implementation is intentionally small and focused on the core OS behavior needed for a parent and one child process to share a page of memory without duplicating it during `fork()`.

---

## 1. Goal

In standard xv6, processes do not share memory by default. Each `fork()` duplicates the parent user memory using `uvmcopy()`, creating a private copy for the child.

The goal here is to make a single page shared between a parent and a child.

That shared page is then used to store the synchronization variables for Peterson's algorithm:

- `flag[2]` — each process signals whether it wants to enter the critical section
- `turn` — tells whose turn it is when both processes want access
- `counter` — shared counter used to verify the critical section is enforced correctly

---

## 2. What was added

### 2.1 Shared memory state in each process

The `struct proc` in [kernel/proc.h](kernel/proc.h) was extended with:

```c
uint64 shm_va;
uint64 shm_pa;
int shm_valid;
```

These fields store:

- the virtual address of the shared page in this process
- the physical address of the page being shared
- whether the process actually owns a valid shared page

---

### 2.2 Shared page allocation syscall

A new syscall was added:

```c
sys_shm_get(void)
```

This function is implemented in [kernel/sysproc.c](kernel/sysproc.c).

It does the following:

1. allocates one physical page with `kalloc()`
2. zeroes it
3. maps it into the current process's user page table at a fixed virtual address `SHMBASE`
4. stores the mapping info in the process structure
5. returns the user pointer to that shared page

The mapping address is defined in [kernel/memlayout.h](kernel/memlayout.h):

```c
#define SHMBASE (TRAPFRAME - PGSIZE)
```

This is placed just below `TRAPFRAME`, so it is not part of the normal heap growth region and is easy to manage separately.

---

### 2.3 Fork behavior

The key change is in [kernel/proc.c](kernel/proc.c), inside `kfork()`.

When the parent already has a valid shared page, the child is created with the same mapping:

```c
if (p->shm_valid) {
  np->shm_va = p->shm_va;
  np->shm_pa = p->shm_pa;
  np->shm_valid = 1;

  if (mappages(np->pagetable, np->shm_va, PGSIZE, np->shm_pa,
              PTE_R | PTE_W | PTE_U) != 0) {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
}
```

This makes the child and parent map the same physical memory, instead of copying it.

That is the essential requirement for shared memory in xv6.

---

### 2.4 Memory cleanup

The shared mapping also has to be cleaned up when a process exits, otherwise a stale page-table entry remains and `freewalk()` later crashes with:

```text
panic: freewalk: leaf
```

This is handled in [kernel/proc.c](kernel/proc.c) by unmapping the shared page before the page table is freed:

```c
uvmunmap(pagetable, SHMBASE, 1, 0);
```

This ensures the shared page does not stay as a live PTE after process exit.

---

## 3. System call registration

The syscall number is added in [kernel/syscall.h](kernel/syscall.h), and the dispatcher is updated in [kernel/syscall.c](kernel/syscall.c):

```c
#define SYS_shm_get 23
```

The user-space stub is generated from [user/usys.pl](user/usys.pl), and the declaration is exposed in [user/user.h](user/user.h).

---

## 4. Peterson's algorithm shared state

Once the shared page exists, the user-level program can cast it to a shared structure:

```c
struct peterson_shared {
  int flag[2];
  int turn;
  int counter;
  int cs_owner;
};
```

The shared variables are:

- `flag[0]` and `flag[1]` — indicate whether each process wants to enter the critical section
- `turn` — the process whose turn it is to enter when both are competing
- `counter` — the shared counter incremented while in the critical section
- `cs_owner` — used as a sanity check to detect overlap of critical sections

These values live in the shared page, so both processes see the same data.

---

## 5. Peterson's algorithm logic

The user program is in [user/peterson.c](user/peterson.c).

Each process runs the standard Peterson entry sequence:

```c
shm->flag[me] = 1;
shm->turn = other;

while (shm->flag[other] == 1 && shm->turn == other)
  ;
```

This says:

- I am interested in entering the critical section
- I give the other process priority by setting `turn`
- if the other process also wants in and it is its turn, I wait

After waiting, the process is guaranteed to be the only one in the critical section at a time.

---

## 6. Critical section in the test program

The test program does this inside the loop:

```c
if (shm->cs_owner != -1) {
  printf("peterson: violation: process %d saw cs_owner=%d\n", me,
         shm->cs_owner);
  exit(1);
}

shm->cs_owner = me;
shm->counter++;
printf("Process %d in CS, counter = %d\n", me, shm->counter);
shm->cs_owner = -1;
shm->flag[me] = 0;
```

This checks two things:

1. the shared counter increments exactly once per critical section entry
2. `cs_owner` is never simultaneously held by two processes, which would indicate a violation

---

## 7. Correctness checks

The test verifies:

- the final value of the shared counter is exactly `2 * ITERATIONS`
- both processes alternate cleanly through the critical section
- no overlapping critical section is observed

At the end:

```c
if (shm->counter != 2 * ITERATIONS) {
  printf("peterson: final counter mismatch: got %d, expected %d\n",
         shm->counter, 2 * ITERATIONS);
  exit(1);
}
```

This ensures there are no lost updates.

---

## 8. Expected behavior

A successful run prints output similar to:

```text
Process 0 in CS, counter = 1
Process 1 in CS, counter = 2
Process 0 in CS, counter = 3
Process 1 in CS, counter = 4
...
Process 0 in CS, counter = 19
Process 1 in CS, counter = 20
peterson: OK, final counter = 20
```

The alternating order is expected because Peterson's algorithm ensures mutual exclusion and fairness among the two processes.

---

## 9. Why this is a good demonstration

This example connects several key OS concepts:

- page-table mapping
- shared physical memory across processes
- `fork()` semantics in xv6
- process memory isolation
- mutual exclusion using a software lock
- correctness checking through a shared counter and ownership flag

It is a very good teaching example because it demonstrates the boundary between:

- normal private process memory, and
- intentionally shared memory between parent and child

---

## 10. Build and test

The program is included in the xv6 build via [Makefile](Makefile).

To rebuild:

```bash
make fs.img
```

Then run xv6 in QEMU and execute the program:

```text
peterson
```

You should see the shared counter increment in a mutually exclusive manner and finish with:

```text
peterson: OK, final counter = 20
```

---

## Summary

This implementation shows a minimal but realistic shared-memory mechanism in xv6, and then proves the correctness of a two-process Peterson algorithm using a shared counter.

It demonstrates both:

- how to share memory between a parent and child in xv6
- how to use that shared memory to implement safe synchronization

