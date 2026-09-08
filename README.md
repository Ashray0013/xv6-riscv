---

# XV6 Counting Semaphore & Producer-Consumer Implementation

This project extends the **xv6-riscv operating system** with kernel-level **counting semaphores** and demonstrates their use through a user-space **Producer-Consumer application**. The implementation ensures safe, deadlock-free synchronization using a UNIX pipe as a bounded buffer.

---

## 📌 Overview

Efficient synchronization in multi-core systems requires mechanisms that prevent race conditions without wasting CPU cycles on busy waiting.  

This project provides:
- **Kernel Semaphores** (`kernel/syssem.c`, `kernel/sem.c`)  
  - Creation, wait (P/Down), and post (V/Up) operations  
  - FIFO fairness to eliminate starvation  
- **Producer-Consumer Demo** (`user/prodcons.c`)  
  - Multi-process application using a UNIX pipe as buffer  
  - Three semaphores (`empty`, `full`, `mutex`) for resource tracking and mutual exclusion  

---

## ⚙️ Kernel Semaphore Design

### Data Structure
```c
#define NSEMAPHORE 32

struct semaphore {
  struct spinlock lock;        // Protects state
  int used;                    // Allocation flag
  int value;                   // Resource count
  int nwaiters;                // Number of waiting processes
  struct proc *waiters[NPROC]; // FIFO queue of waiters
};

static struct semaphore semaphores[NSEMAPHORE];
```

### Primitives
- **`semcreate(int value)`** → Allocates and initializes a semaphore.  
- **`semwait(int id)`** → Decrements counter or blocks process in FIFO order using `sleep()`. Handles cancellation safely.  
- **`sempost(int id)`** → Increments counter and wakes the head of the wait queue.  

---

## 🧩 Producer-Consumer Synchronization

### Semaphores
| Name     | Initial Value | Purpose                          |
|----------|---------------|----------------------------------|
| `empty`  | buffer_size   | Tracks empty slots               |
| `full`   | 0             | Tracks filled items              |
| `mutex`  | 1             | Ensures mutual exclusion         |

### Flow
**Producer**
1. `sem_wait(empty)`  
2. `sem_wait(mutex)`  
3. `write(fd, &item)`  
4. `sem_post(mutex)`  
5. `sem_post(full)`  

**Consumer**
1. `sem_wait(full)`  
2. `sem_wait(mutex)`  
3. `read(fd, &item)`  
4. `sem_post(mutex)`  
5. `sem_post(empty)`  

### Deadlock Prevention
- **Rule:** Decrement resource semaphores (`empty`, `full`) **before** acquiring `mutex`.  
- **Why:** If `mutex` is held while waiting on `empty`, the consumer cannot acquire `mutex` to drain the buffer → deadlock.  

---

## 🚀 Build & Run

### Compilation
1. Add `_prodcons` to `UPROGS` in `Makefile`.  
2. Register system calls (`sys_sem_create`, `sys_sem_wait`, `sys_sem_post`) in:
   - `kernel/syscall.h`  
   - `kernel/syscall.c`  
   - `user/user.h`  
3. Build and launch QEMU:
   ```bash
   make qemu
   ```

### Execution
Inside xv6 shell:
```bash
$ prodcons 5
```

---

## ✅ Features & Edge Cases

- **No Busy Waiting** → Uses `sleep()` / `wakeup()` for efficient blocking  
- **FIFO Fairness** → Guarantees bounded waiting, no starvation  
- **Safe Termination** → Processes killed while waiting are removed cleanly from the queue  

---
