#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define MAX_READER_BATCH 3

static struct spinlock rw_lock;
static int read_count;
static int waiting_writers;
static int writer_active;
static int reader_batch;
static int shared_data;

void
rwinit(void)
{
  initlock(&rw_lock, "rwlock");
  read_count = 0;
  waiting_writers = 0;
  writer_active = 0;
  reader_batch = 0;
  shared_data = 0;
}

void
rw_read_enter(void)
{
  for (;;) {
    acquire(&rw_lock);
    if (!writer_active &&
        (waiting_writers == 0 || reader_batch < MAX_READER_BATCH)) {
      read_count++;
      reader_batch++;
      release(&rw_lock);
      return;
    }
    release(&rw_lock);
    yield();
  }
}

void
rw_read_exit(void)
{
  acquire(&rw_lock);
  read_count--;
  release(&rw_lock);
}

void
rw_write_enter(void)
{
  acquire(&rw_lock);
  waiting_writers++;
  release(&rw_lock);

  for (;;) {
    acquire(&rw_lock);
    if (!writer_active && read_count == 0) {
      writer_active = 1;
      waiting_writers--;
      reader_batch = 0;
      release(&rw_lock);
      return;
    }
    release(&rw_lock);
    yield();
  }
}

void
rw_write_exit(void)
{
  acquire(&rw_lock);
  writer_active = 0;
  release(&rw_lock);
}

int
rw_get_data(void)
{
  int value;

  acquire(&rw_lock);
  value = shared_data;
  release(&rw_lock);
  return value;
}

int
rw_increment_data(void)
{
  int value;

  acquire(&rw_lock);
  shared_data++;
  value = shared_data;
  release(&rw_lock);
  return value;
}