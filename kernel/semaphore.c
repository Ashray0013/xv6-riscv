#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define NSEMAPHORE 32

struct semaphore {
  struct spinlock lock;
  int used;
  int value;
  int nwaiters;
  struct proc *waiters[NPROC];
};

static struct semaphore semaphores[NSEMAPHORE];

static void
wake_one(struct semaphore *sem, struct proc *p)
{
  acquire(&p->lock);
  if (p->chan == sem) {
    p->chan = 0;
    if (p->state == SLEEPING)
      p->state = RUNNABLE;
  }
  release(&p->lock);
}

void
seminit(void)
{
  for (int i = 0; i < NSEMAPHORE; i++) {
    initlock(&semaphores[i].lock, "semaphore");
    semaphores[i].used = 0;
    semaphores[i].value = 0;
    semaphores[i].nwaiters = 0;
  }
}

int
semcreate(int value)
{
  if (value < 0)
    return -1;

  for (int i = 0; i < NSEMAPHORE; i++) {
    acquire(&semaphores[i].lock);
    if (!semaphores[i].used) {
      semaphores[i].used = 1;
      semaphores[i].value = value;
      semaphores[i].nwaiters = 0;
      release(&semaphores[i].lock);
      return i;
    }
    release(&semaphores[i].lock);
  }
  return -1;
}

int
semwait(int id) // P() or Dowm()
{
  struct proc *p = myproc();
  struct semaphore *sem;

  if (id < 0 || id >= NSEMAPHORE)
    return -1;
  sem = &semaphores[id];

  acquire(&sem->lock);
  if (!sem->used) {
    release(&sem->lock);
    return -1;
  }

  // The negative value counts processes waiting for this semaphore.
  sem->value--;
  if (sem->value < 0) {
    if (sem->nwaiters == NPROC) {
      sem->value++;
      release(&sem->lock);
      return -1;
    }
    sem->waiters[sem->nwaiters++] = p;
    sleep_prepare(sem);
    release(&sem->lock);
    sleep();
    return 0;
  }

  release(&sem->lock);
  return 0;
}

int
sempost(int id) //V() or up()
{
  struct semaphore *sem;

  if (id < 0 || id >= NSEMAPHORE)
    return -1;
  sem = &semaphores[id];

  acquire(&sem->lock);
  if (!sem->used) {
    release(&sem->lock);
    return -1;
  }

  sem->value++;
  if (sem->value <= 0 && sem->nwaiters > 0) {
    struct proc *p = sem->waiters[0];

    for (int i = 0; i + 1 < sem->nwaiters; i++)
      sem->waiters[i] = sem->waiters[i + 1];
    sem->nwaiters--;
    sem->waiters[sem->nwaiters] = 0;
    wake_one(sem, p);
  }
  release(&sem->lock);
  return 0;
}
