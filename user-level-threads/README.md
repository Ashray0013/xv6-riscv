# 2402MC08_asg_07_MA3104 — OS Lab Assignment 7 (xv6)

> **Course:** MA3104 (Operating Systems)
> **Student:** Shaan Shan | 2402MC08
> **Platform:** xv6 (public) on QEMU, with kernel extensions

## Overview

Four experiments built directly on the xv6 kernel, one per classic OS topic:

| # | Topic | What was built |
|---|-------|----------------|
| Q1 | Threads | `clone()` / `join()` syscalls — kernel-supported threads sharing one address space |
| Q2 | Synchronization | Counting semaphores (`sem_init/wait/signal`) fix a real race + producer-consumer |
| Q3 | Deadlock | User-space detection (work-vector) + Banker avoidance + a real pipe deadlock |
| Q4 | Scheduling | Priority-preemptive scheduler with aging replaces round-robin |

## Repository layout

    q1/   clone/join threads: threadtest.c + kernel diffs + README + log
    q2/   semaphores: syntest.c (exact counter, producer-consumer) + diffs + log
    q3/   deadlock.c: 3 test matrices + real pipe deadlock + diffs + log
    q4/   priority scheduler: priotest.c + trap/proc/scheduler diffs + log
    REPORT.md   consolidated 2-3 page report (design, results, problems faced)

## Quick start (any question)

    cd <question folder sources>   # see its README
    make clean && make qemu-nox
    $ threadtest / syntest / deadlock / priotest

## Headline results

- **Q1:** 3 threads, one shared counter -> proves shared address space; unsynchronized value unreliable.
- **Q2:** with semaphores the counter is **exactly 300000 every run**; producer-consumer log shows no overflow/underflow.
- **Q3:** detection flags the deadlocked matrix {P0,P1}, passes the safe one, denies the unsafe request; real A<->B pipe circular wait identified and broken.
- **Q4:** completion order follows priority (1,5,10,20 -> ~100,110,200,300 ticks) and the prio-20 process survives via aging (20->19->18).
