# vmprint: a page-table dump system call for xv6-riscv

`vmprint()` is a system call that walks the calling process's page table and
prints every valid user page: its virtual address, the physical address of its
frame, and the P, W and U flags.

This is an add-on for [xv6-riscv](https://github.com/mit-pdos/xv6-riscv), MIT's
teaching operating system.

## Interface

```c
int vmprint(void);   // user-level; prints the caller's user mappings, returns 0
```

## How it works

xv6-riscv uses the RISC-V **Sv39** scheme: three levels of page tables, each a
4 KB page holding 512 entries (PTEs). A virtual address splits into:

```
bits 30-38  level-2 index (9 bits)
bits 21-29  level-1 index (9 bits)
bits 12-20  level-0 index (9 bits)
bits  0-11  byte offset within the page
```

The walker is modeled on `freewalk()` in `kernel/vm.c`:

1. Loop over all 512 entries of a table and skip entries without `PTE_V`.
2. Rebuild the entry's virtual address as `va_prefix | (i << PXSHIFT(level))`.
3. If the entry is valid and has none of `PTE_R | PTE_W | PTE_X`, it points to
   a lower-level table, so recurse with `level - 1`.
4. Otherwise it is a leaf. If `PTE_U` is set, print it.

Filtering on `PTE_U` is what ignores the "kernel part" of the address space. A
user page table contains no kernel mappings, but it does contain the trampoline
and trapframe pages and the stack guard page. None of those have `PTE_U`, so
they are skipped.

### Flag mapping

The original assignment is phrased for x86 xv6. On RISC-V:

| Assignment | This implementation |
|------------|---------------------|
| P          | `PTE_V`             |
| W          | `PTE_W`             |
| U          | `PTE_U`             |

## Files changed

| File | Change |
|------|--------|
| `kernel/syscall.h` | `#define SYS_vmprint 23` |
| `kernel/syscall.c` | `extern` declaration and `[SYS_vmprint] = sys_vmprint` table entry |
| `kernel/sysproc.c` | `sys_vmprint()` calls `vmprint(myproc()->pagetable)` |
| `kernel/vm.c` | `vmprint_walk()` (recursive helper) and `vmprint()` |
| `kernel/defs.h` | prototype `int vmprint(pagetable_t);` |
| `user/user.h` | prototype `int vmprint(void);` |
| `user/usys.pl` | `entry("vmprint");` |
| `user/vmprinttest.c` | test program (new file) |
| `Makefile` | `$U/_vmprinttest\` added to `UPROGS` |

## Build and run

Requirements: a RISC-V GCC toolchain and QEMU (on Debian/Ubuntu:
`gcc-riscv64-linux-gnu` and `qemu-system-misc`).

```
make clean
make qemu
```

At the xv6 shell:

```
$ vmprinttest
```

Exit QEMU with `Ctrl-A` then `X`.

## Sample output

```
$ vmprinttest
--- before sbrk ---
va 0x0 -> pa 0x87f22000  flags: P=1 W=0 U=1
va 0x1000 -> pa 0x87f1f000  flags: P=1 W=1 U=1
va 0x3000 -> pa 0x87f1d000  flags: P=1 W=1 U=1
--- after sbrk(3 pages) ---
va 0x0 -> pa 0x87f22000  flags: P=1 W=0 U=1
va 0x1000 -> pa 0x87f1f000  flags: P=1 W=1 U=1
va 0x3000 -> pa 0x87f1d000  flags: P=1 W=1 U=1
va 0x4000 -> pa 0x87f26000  flags: P=1 W=1 U=1
va 0x5000 -> pa 0x87f53000  flags: P=1 W=1 U=1
va 0x6000 -> pa 0x87f49000  flags: P=1 W=1 U=1
```

### Reading the output

| Virtual address | What it is | Flags |
|-----------------|-----------|-------|
| `0x0`           | program code (text), read-only and executable | W=0 |
| `0x1000`        | initialized data and bss | W=1 |
| `0x2000`        | stack **guard page**, not listed (`PTE_U` cleared by `uvmclear`) | n/a |
| `0x3000`        | user stack | W=1 |
| `0x4000`-`0x6000` | three heap pages added by `sbrk(3 * 4096)` | W=1 |

Physical addresses all fall in `0x80000000` and above, which is the RAM region
the kernel hands out through `kalloc()`. They are not contiguous, because
physical pages come from a free list.

## Common problems

| Symptom | Cause |
|---------|-------|
| `unknown sys call 23` | missing `[SYS_vmprint] = sys_vmprint,` in the `syscalls[]` table in `kernel/syscall.c`, or an old kernel is being booted after a failed build |
| `implicit declaration of function 'vmprint'` | missing `int vmprint(pagetable_t);` in `kernel/defs.h` |
| `undefined reference to 'vmprint'` in a user program | missing `entry("vmprint");` in `user/usys.pl`; run `make clean` so `usys.S` is regenerated |
| `missing separator` from make | the `UPROGS` line must start with a tab, not spaces |

## Notes

- Only P, W and U are printed. R and X could be added the same way, from
  `PTE_R` and `PTE_X`.
- The process's page table is private to it, so the walk needs no locking.
- The calling process's own mappings are what is shown, so output depends on
  the program that calls `vmprint()`.
