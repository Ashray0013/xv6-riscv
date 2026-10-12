Asg 7 Q4: Priority-based preemptive scheduling with aging (xv6 kernel)

struct proc gains: priority (1 highest .. 20 lowest, default 10 set in
allocproc) and waitticks (ticks spent RUNNABLE but not scheduled).
Syscalls: set_priority(pid,prio) = 22, get_priority(pid) = 23, both
validated (1..20) and performed under the ptable lock.

scheduler() (proc.c): on every scheduling decision it scans the proc table
from a rotating cursor rr_idx and picks the RUNNABLE process with the
LOWEST priority number; equal priorities are broken round-robin because the
scan starts at rr_idx and rr_idx advances past the chosen process. The
dispatched process gets waitticks = 0.

Aging (proc.c age_procs, called from trap.c on every timer tick): each
RUNNABLE process's waitticks increments; above 30 ticks the priority number
is decremented by 1 (boost) and the counter resets - a starvable process
climbs until it runs.

priotest.c: 4 CPU-bound children (100 ticks of work each) with priorities
1, 5, 10, 20. Each prints start, periodic (tick, current priority) and
FINISHED lines.

OBSERVED (q4_priotest.png): completion order proc0(prio1) and proc1(prio5)
first (~tick 100-110 on 2 CPUs), then proc2(prio10) ~tick 200, then
proc3(prio20) ~tick 300. proc3's periodic lines show its priority stepping
20 -> 19 -> 18 while it waits (aging), proving starvation is prevented: the
lowest-priority process still completes. Completion order correlates with
priority as required.

BUILD & RUN: make clean && make qemu-nox ; $ priotest
