#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define ITERATIONS 10

struct peterson_shared {
  int flag[2];
  int turn;
  int counter;
  int cs_owner;
};

static void
remainder_work(void)
{
  volatile int i;
  volatile int sum = 0;

  for (i = 0; i < 20000; i++)
    sum += i;
}

int
main(int argc, char **argv)
{
  volatile struct peterson_shared *shm;
  int pid;
  int me;
  int other;
  int i;

  (void)argc;
  (void)argv;

  shm = (volatile struct peterson_shared *)shm_get();
  if ((char *)shm == (char *)-1) {
    printf("peterson: shm_get failed\n");
    exit(1);
  }

  shm->flag[0] = 0;
  shm->flag[1] = 0;
  shm->turn = 0;
  shm->counter = 0;
  shm->cs_owner = -1;

  pid = fork();
  if (pid < 0) {
    printf("peterson: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    me = 1;
    other = 0;
  } else {
    me = 0;
    other = 1;
  }

  for (i = 0; i < ITERATIONS; i++) {
    shm->flag[me] = 1;
    shm->turn = other;

    while (shm->flag[other] == 1 && shm->turn == other)
      ;

    if (shm->cs_owner != -1) {
      printf("peterson: violation: process %d saw cs_owner=%d\n", me,
             shm->cs_owner);
      exit(1);
    }

    shm->cs_owner = me;
    shm->counter++;
    printf("Process %d in CS, counter = %d\n", me, shm->counter);
    shm->cs_owner = -1;
    shm->flag[me] = 0;

    remainder_work();
  }

  if (pid == 0)
    exit(0);

  if (wait(0) < 0) {
    printf("peterson: wait failed\n");
    exit(1);
  }

  if (shm->counter != 2 * ITERATIONS) {
    printf("peterson: final counter mismatch: got %d, expected %d\n",
           shm->counter, 2 * ITERATIONS);
    exit(1);
  }

  printf("peterson: OK, final counter = %d\n", shm->counter);
  exit(0);
}
