#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREADS 3
#define INCS     100000
#define STACKSZ  4096

static int counter = 0;    // shared global: visible to all threads

void
worker(void *arg)
{
  int i, id = (int)arg;
  for(i = 0; i < INCS; i++){
    if(i % 1000 == 0){
      int v = counter;     // read shared counter
      sleep(1);            // preempt BETWEEN read and write
      counter = v + 1;     // write STALE value -> other updates lost
    } else {
      counter++;           // still a non-atomic read-modify-write
    }
  }
  printf(1, "thread %d finished\n", id);
  exit();
}

int
main(void)
{
  int i;
  void *stacks[NTHREADS];

  for(i = 0; i < NTHREADS; i++){
    stacks[i] = malloc(STACKSZ);
    if(!stacks[i]){ printf(1, "malloc failed\n"); exit(); }
    if(clone(worker, (void *)i, stacks[i]) < 0){
      printf(1, "clone failed\n");
      exit();
    }
  }
  for(i = 0; i < NTHREADS; i++){
    if(join() < 0){ printf(1, "join failed\n"); break; }
  }
  printf(1, "final counter = %d (max possible %d)\n", counter, NTHREADS*INCS);
  if(counter == NTHREADS*INCS)
    printf(1, "no races this run\n");
  else
    printf(1, "races lost updates (expected without synchronization)\n");
  exit();
}
