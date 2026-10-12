Asg 7 Q2: Counting semaphores + thread synchronization in xv6

KERNEL SEMAPHORES (proc.c)
struct semaphore { int value; struct spinlock lk; int inuse; };
Fixed array sems[10] in kernel space; every access to a slot happens under
that slot's own spinlock.
sem_wait(id):  acquire; while(value <= 0) sleep(&sems[id], &lk); value--;
               release.  Blocking/waking uses xv6 sleep()/wakeup() on the
               semaphore address; the while-recheck makes wakeup-all safe.
sem_signal(id): acquire; value++; wakeup(&sems[id]); release.
sem_init(id, v): sets value and marks the slot in use.
System calls: sem_init = 24, sem_wait = 25, sem_signal = 26
(syscall.h, syscall.c dispatch table, sysproc.c wrappers, usys.S stubs).

SYNTEST.C PART 1 - shared counter, now synchronized
3 threads x 100,000 increments of a shared global; every increment wrapped
in sem_wait(SEM_COUNTER) ... sem_signal(SEM_COUNTER) (Q1's clone/join
threads reused).

COMPARISON WITH Q1 (assignment expected outcome)
Q1 threadtest (NO synchronization): counter++ is a non-atomic
read-modify-write on shared memory; whenever two threads overlap, updates
are lost and the final value reads LESS than 300000 with the log line
"races lost updates" (see q1 log / threadtest run). Even a serialized
single-CPU run that happens to print 300000 carries no guarantee - the
program is simply lucky.
Q2 syntest (WITH semaphores): the increment lives inside a mutual-exclusion
critical section, so EVERY run on EVERY schedule prints exactly
"counter with semaphores = 300000 (expected exactly 300000)".
Observed log (q2_syntest.png): all three threads finish their 100000
increments and the parent reports the exact value - synchronization verified
against Q1's incorrect/racy behaviour.

SYNTEST.C PART 2 - Producer-Consumer, bounded buffer of size 5
Semaphores: EMPTY = 5, FULL = 0, BUF (mutex) = 1.
Producer thread: wait(EMPTY), wait(BUF), insert item, signal(BUF),
signal(FULL).  Consumer thread: wait(FULL), wait(BUF), remove item,
signal(BUF), signal(EMPTY).  20 items, two threads via clone/join.

NO OVERFLOW / UNDERFLOW - VERIFIED BY PRINTED LOG (q2_syntest.png)
Every produced/consumed item is logged with its buffer slot. The producer
fills slots 0-4 and then blocks on EMPTY (never a 6th unconsumed item =
no overflow); the consumer blocks on FULL when the buffer is empty (never
reads garbage = no underflow). Items come out in FIFO order 1..20, each
exactly once; "producer done", "consumer done" and the final line
"producer-consumer completed with no overflow/underflow" confirm clean
termination.

BUILD & RUN
make clean && make qemu-nox
$ syntest        synchronized counter + producer-consumer
$ threadtest     Q1's unsynchronized version, for comparison

FILES: syntest.c; modified proc.c, sysproc.c, syscall.c, syscall.h, defs.h,
user.h, usys.S, Makefile; diffs in diffs/; log screenshot q2_syntest.png.
