Asg 7 Q1: Kernel-supported user-level threads (clone / join) in xv6

MOTIVATION
xv6 only supports fork(), which creates a process with a private address
space. This question adds kernel-supported threads: lightweight procs that
share the creator's page table (pgdir) but own a private kernel stack.

DESIGN
struct proc gains an "isthread" flag.

clone(fcn, arg, stack):
  - allocproc() provides a fresh proc slot with its own kernel stack;
  - pgdir and sz are SHARED with the caller (no copyuvm) => one address space;
  - the open-file table is duplicated (filedup) so thread printf/read work;
  - the trapframe is programmed so the thread enters user mode at fcn(arg):
    esp = top of the user-provided stack page, with arg and a fake return
    address pushed in cdecl layout; eip = fcn; state -> RUNNABLE.

join():
  - like wait(), but reaps ONLY isthread children;
  - frees the thread's kernel stack but NEVER freevm's the pgdir (shared).

exit()/wait()/fork() correctness:
  - wait() skips isthread children, so it can never free the shared pgdir;
    threads are reaped exclusively by join();
  - exit() needs no freeing change (xv6 frees address spaces in wait/join),
    and wakeup1(parent) releases a parent sleeping inside join();
  - fork() by a thread produces a normal process: private copyuvm copy,
    isthread = 0.

Syscall wiring: clone = 22, join = 23 (syscall.h, syscall.c table,
sysproc.c wrappers, usys.S stubs, defs.h/user.h prototypes).

TEST (threadtest.c)
3 threads each increment a shared global counter 100,000 times, then the
parent joins all three and prints the final value.
  - counter > 0 in the parent proves a single shared address space
    (a fork-style private copy would print 0);
  - the value being below 300,000 (when it occurs) demonstrates lost
    updates from unsynchronized concurrent increments - the setup for Q2.
  - the worker yields (sleep(1)) periodically so threads interleave even
    on single-CPU QEMU, widening the race window.

OBSERVED RESULT (see q1_threadtest.png)
All three threads print "thread N finished" from their own execution
context, join() returns for each, and the parent prints
"final counter = 300000 (max possible 300000)" for the serialized run;
other runs show lower values with "races lost updates", confirming both
shared memory and the race condition.

BUILD & RUN
make clean && make qemu-nox
$ threadtest
