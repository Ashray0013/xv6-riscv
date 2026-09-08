#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DEFAULT_BUFFER_SIZE 3
#define ITEM_COUNT          10

static void
producer(int fd, int empty, int full, int mutex)
{
  for (int item = 1; item <= ITEM_COUNT; item++) {
    printf("[PRODUCER] Trying to produce item %d...\n", item);

    // Will block here if empty == 0 (Buffer Full)
    if (sem_wait(empty) < 0)
      exit(1);

    if (sem_wait(mutex) < 0)
      exit(1);

    // CRITICAL SECTION: Writing to buffer
    printf("[PRODUCER] -> Produced item %d\n", item);
    if (write(fd, &item, sizeof(item)) != sizeof(item))
      exit(1);

    if (sem_post(mutex) < 0 || sem_post(full) < 0)
      exit(1);

    // Faster production speed so buffer fills up to demonstrate blocking
    pause(1);
  }
  close(fd);
  exit(0);
}

static void
consumer(int fd, int empty, int full, int mutex)
{
  int item;

  for (int i = 0; i < ITEM_COUNT; i++) {
    printf("[CONSUMER] Trying to consume item...\n");

    // Will block here if full == 0 (Buffer Empty)
    if (sem_wait(full) < 0)
      exit(1);

    if (sem_wait(mutex) < 0)
      exit(1);

    // CRITICAL SECTION: Reading from buffer
    if (read(fd, &item, sizeof(item)) != sizeof(item))
      exit(1);
    printf("  [CONSUMER] <- Consumed item %d\n", item);

    if (sem_post(mutex) < 0 || sem_post(empty) < 0)
      exit(1);

    // Slower consumption speed to force buffer to reach capacity
    pause(5);
  }
  close(fd);
  exit(0);
}

int
main(int argc, char *argv[])
{
  int buffer_size = DEFAULT_BUFFER_SIZE;
  int fds[2];
  int empty;
  int full;
  int mutex;
  int producer_pid;
  int consumer_pid;

  if (argc > 1)
    buffer_size = atoi(argv[1]);
  if (buffer_size <= 0)
    buffer_size = DEFAULT_BUFFER_SIZE;

  printf("prodcons: starting with buffer size %d\n", buffer_size);

  if (pipe(fds) < 0)
    exit(1);

  empty = sem_create(buffer_size);
  full = sem_create(0);
  mutex = sem_create(1);
  if (empty < 0 || full < 0 || mutex < 0)
    exit(1);

  producer_pid = fork();
  if (producer_pid == 0) {
    close(fds[0]);
    producer(fds[1], empty, full, mutex);
  }
  if (producer_pid < 0)
    exit(1);

  consumer_pid = fork();
  if (consumer_pid == 0) {
    close(fds[1]);
    consumer(fds[0], empty, full, mutex);
  }
  if (consumer_pid < 0)
    exit(1);

  close(fds[0]);
  close(fds[1]);
  wait(0);
  wait(0);
  printf("producer-consumer completed successfully\n");
  exit(0);
}