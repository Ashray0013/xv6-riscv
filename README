# xv6 RISC-V Reader-Writer Demonstration

This repository contains a small xv6 operating-system environment and a
reader-writer synchronization demonstration. The demonstration implements the
first Readers-Writers problem: readers are allowed to share access, while a
writer requires exclusive access. It also bounds the number of readers that
can pass when a writer is waiting, so writers cannot starve indefinitely.

## What Is Implemented

The demonstration is composed of these files:

| File | Purpose |
| --- | --- |
| `user/readwrite.c` | Forks three reader processes and two writer processes. It performs the test and prints PID/timestamp logs. |
| `kernel/rwlock.c` | Stores the shared integer and implements the reader-writer state machine using an xv6 spinlock. |
| `kernel/defs.h` | Declares the kernel reader-writer functions. |
| `kernel/main.c` | Initializes the reader-writer state during kernel startup. |
| `kernel/sysproc.c` | Implements the system-call handlers. |
| `kernel/syscall.c` | Registers the system calls with the kernel dispatcher. |
| `kernel/syscall.h` | Assigns system-call numbers. |
| `user/user.h` | Declares the reader-writer calls for user programs. |
| `user/usys.pl` | Generates the user-level assembly stubs. |
| `Makefile` | Links `kernel/rwlock.o` and installs `readwrite` in the filesystem image. |

## Why The Shared State Is In The Kernel

The assignment requires a shared integer named `shared_data`. In this xv6
version, a normal `fork()` creates a separate user address space for each
child by copying the parent's memory. Therefore, a global variable declared
only in `user/readwrite.c` would not be shared between the readers and writers:
each child would modify or read its own copy.

To make the state genuinely shared, `kernel/rwlock.c` owns:

```c
static int shared_data;
static int read_count;
```

The user processes access this state through system calls. This also puts the
synchronization primitive in the kernel, where all processes can use the same
lock and the same counters.

## Synchronization State

`kernel/rwlock.c` maintains the following fields under the `rw_lock` spinlock:

- `shared_data`: the integer incremented by writers and read by readers.
- `read_count`: the number of readers currently inside a read section.
- `waiting_writers`: the number of writers that have requested access but are
  not yet inside a write section.
- `writer_active`: nonzero while a writer owns exclusive access.
- `reader_batch`: the number of readers admitted since the current writer
  contention period began.

The spinlock protects transitions of all these fields. Processes that cannot
enter call `yield()` and retry, allowing another process to run instead of
busy-spinning continuously.

## Reader Algorithm

When a reader calls `rw_read_enter()`:

1. It acquires `rw_lock`.
2. It may enter if no writer is active and either no writer is waiting or the
   current reader batch is smaller than `MAX_READER_BATCH`.
3. It increments `read_count` and `reader_batch`.
4. It releases `rw_lock` and performs its read section concurrently with any
   other admitted readers.

The reader then calls `rw_read_exit()`, which decrements `read_count` while
holding the spinlock.

The read section intentionally includes a short `pause(2)` call. This makes
overlap visible in the output: multiple readers can remain active while they
print the same value of `shared_data`.

## Writer Algorithm

When a writer calls `rw_write_enter()`:

1. It increments `waiting_writers` while holding `rw_lock`.
2. It waits until `writer_active == 0` and `read_count == 0`.
3. It sets `writer_active` to one and decrements `waiting_writers`.
4. It resets `reader_batch` to zero for the next contention period.

After entry, the writer increments `shared_data` through
`rw_increment_data()`. It then logs the new value and briefly pauses while
still holding the write-side ownership. Finally, `rw_write_exit()` clears
`writer_active`.

Because a writer enters only when `read_count` is zero, no reader can be
active during the increment. Because `writer_active` is checked by all readers
and writers, two writers cannot enter together either.

## Reader Priority And Fairness

The traditional readers-priority solution lets new readers join existing
readers whenever no writer currently owns the resource. This gives readers
priority, but it has a weakness: if readers continuously arrive, a writer can
wait forever.

This implementation keeps the reader-priority behavior when there is no writer
contention. Once at least one writer is waiting, only three readers from the
current batch may enter:

```c
#define MAX_READER_BATCH 3
```

After that batch finishes, new readers wait for a writer. The writer can enter
when `read_count` reaches zero. This is a bounded-reader or fair reader-
priority variation: readers still get preference when uncontended, but a
waiting writer is guaranteed an opportunity after a finite reader batch.

This is preferable to an unbounded readers-priority implementation for the
assignment because it demonstrates both reader concurrency and protection
against indefinite writer starvation.

## User Workload

`user/readwrite.c` defines:

```c
#define NREADERS     3
#define NWRITERS     2
#define READ_ROUNDS  4
#define WRITE_ROUNDS 4
```

The parent process forks three readers and then two writers. Each reader runs
four rounds. Each writer also runs four rounds, so the expected final value is:

```text
2 writers * 4 increments per writer = 8
```

The parent waits for all five children and then prints the final value of
`shared_data`.

## Logging

Each event contains:

- `t`: the xv6 uptime tick when the event was logged;
- `pid`: the process ID of the reader or writer;
- the operation and phase;
- the observed or newly written `shared_data` value;
- `(exclusive)` on writer events.

The program builds each event in a local buffer and sends it with one user
`write()` call. This is better than calling xv6 `printf()` for the event,
because xv6's `printf()` emits individual characters. However, the UART and
console output path can still interleave output from multiple processes while
the machine is scheduling them. Therefore, occasional visual mixing in QEMU
output does not imply that the reader-writer lock failed. The synchronization
properties should be checked using the values and ordering, not only by
whether every terminal line appears perfectly formatted.

Representative output is:

```text
reader-writer test: 3 readers, 2 writers
[t=236 pid=4] READ  start shared_data=0
[t=236 pid=5] READ  start shared_data=0
[t=236 pid=6] READ  start shared_data=0
[t=238 pid=4] READ  end   shared_data=0
[t=238 pid=5] READ  end   shared_data=0
[t=238 pid=6] READ  end   shared_data=0
[t=238 pid=7] WRITE start/end shared_data=1 (exclusive)
[t=239 pid=6] READ  start shared_data=1
[t=239 pid=4] READ  start shared_data=1
[t=239 pid=5] READ  start shared_data=1
[t=241 pid=6] READ  end   shared_data=1
[t=241 pid=4] READ  end   shared_data=1
[t=241 pid=5] READ  end   shared_data=1
[t=241 pid=7] WRITE start/end shared_data=2 (exclusive)
[t=242 pid=8] WRITE start/end shared_data=3 (exclusive)
[t=243 pid=4] READ  start shared_data=3
[t=243 pid=5] READ  start shared_data=3
[t=243 pid=6] READ  start shared_data=3
[t=245 pid=4] READ  end   shared_data=3
[t=245 pid=6] READ  end   shared_data=3
[t=245 pid=5] READ  end   shared_data=3
[t=245 pid=7] WRITE start/end shared_data=4 (exclusive)
[t=248 pid=7] WRITE start/end shared_data=5 (exclusive)
[t=249 pid=8] WRITE start/end shared_data=6 (exclusive)
[t=252 pid=8] WRITE start/end shared_data=7 (exclusive)
[t=255 pid=8] WRITE start/end shared_data=8 (exclusive)
reader-writer test complete: final shared_data=8
```

The exact timestamps, process IDs, and scheduling order vary between runs.
The important observations are:

1. Multiple readers can start before any of them finishes.
2. Concurrent readers observe the same value during a read phase.
3. Writer values increase one at a time: 1, 2, 3, and so on.
4. A writer enters only after the previous readers have left.
5. The final value is 8.

## Requirements Checklist

- Shared integer `shared_data`: implemented in `kernel/rwlock.c`.
- `read_count` protected by a mutex: implemented under the xv6 `rw_lock`
  spinlock.
- Multiple concurrent readers: three reader processes are forked and admitted
  together.
- Exclusive writers: `rw_write_enter()` waits for zero active readers and no
  active writer.
- At least three readers and two writers: exactly three readers and two writers
  are created.
- PID and timestamp output: every reader/writer event includes `getpid()` and
  `uptime()` values.
- Writer starvation prevention: the reader batch is limited while a writer is
  waiting.

## Building And Running

### Prerequisites

Install or provide these tools in a shell environment:

- a RISC-V GCC/binutils toolchain, such as `riscv64-linux-gnu-gcc`,
- GNU `make`,
- Perl, used to generate `user/usys.S`,
- QEMU with `qemu-system-riscv64`.

The original xv6 instructions reference the RISC-V toolchain at:

<https://github.com/riscv/riscv-gnu-toolchain>

### Build The Kernel And Program

From the repository root:

```sh
make kernel/kernel
make user/_readwrite fs.img
```

To rebuild everything needed for a normal run:

```sh
make qemu
```

### Run The Test

When the xv6 shell appears, run:

```text
$ readwrite
```

Exit QEMU with the terminal interrupt sequence supported by your environment,
usually `Ctrl-a` followed by `x` for QEMU's `-nographic` mode.

### Run Through WSL On Windows

If the tools are installed in WSL and the repository is on the Windows C:
drive, use:

```powershell
wsl --cd /mnt/c/Users/Samsung/Desktop/xv6-riscv-1 make qemu
```

Then type `readwrite` at the xv6 shell prompt.

Only one QEMU process should use `fs.img` at a time. If QEMU reports that it
cannot obtain the write lock for `fs.img`, stop the previous QEMU instance and
retry.

## Clean Build

Use this command from an environment with GNU make:

```sh
make clean
```

On Windows PowerShell, `make` may not be installed even when it is available
inside WSL. In that case, run the command through WSL:

```powershell
wsl --cd /mnt/c/Users/Samsung/Desktop/xv6-riscv-1 make clean
```

## Original xv6 Information

xv6 is a reimplementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6. It follows the structure and style of v6 while targeting a modern
RISC-V multiprocessor using ANSI C. xv6 is intended as a teaching operating
system for MIT's 6.1810 course.

The upstream xv6 project and course materials are available at:

<https://pdos.csail.mit.edu/6.1810/>
