#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("--- before sbrk ---\n");
  vmprint();

  sbrk(3 * 4096);
  printf("--- after sbrk(3 pages) ---\n");
  vmprint();

  exit(0);
}