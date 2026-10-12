Asg 7 Q3: Deadlock detection + avoidance (user-space, xv6 as environment)

MODEL (deadlock.c): static arrays Allocation[n][m], Request[n][m],
Max[n][m], Available[m]; Need = Max - Allocation. State (Available,
Allocation, Request, Need) printed at every step.

detect_deadlock(): work-vector algorithm - repeatedly finish any process
whose Request <= work and add its Allocation back to work; processes left
unfinished are returned as the deadlocked set.
Scenario 1 (safe): all three finish -> NO DEADLOCK.
Scenario 2 (deadlocked): P0 holds R0 wants R1, P1 holds R1 wants R0,
Available=0 -> nobody finishes -> DEADLOCK {P0,P1}.

REAL DEADLOCK (Scenario 4): two forked xv6 processes and two pipes.
A keeps Pipe1's write end and blocks reading Pipe2; B keeps Pipe2's write
end and blocks reading Pipe1 - a genuine circular wait inside xv6. The
parent models the holdings as matrices, detect_deadlock() identifies both
processes (circular wait A -> B -> A), then kill() breaks the deadlock and
wait() reaps them (piperead honors p->killed).

AVOIDANCE (Scenario 3, Banker): is_safe_state(pid, req) checks req <= Need
and req <= Available, pretends to grant, runs safety_check() (Need-based
work vector), rolls back. On the test state (Available 1,1; Alloc 1,1 each;
Max P0=3,3 P1=2,2) the state is safe; P1's request (1,1) stays safe ->
grantable; P0's identical request leaves Available 0,0 with both Needs
positive -> UNSAFE -> DENIED, i.e. the state would become unsafe after it.

BUILD & RUN: make clean && make qemu-nox ; $ deadlock
LOG: q3_deadlock.png (all four scenarios with printed states).
