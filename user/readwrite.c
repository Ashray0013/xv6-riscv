#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NREADERS     3
#define NWRITERS     2
#define READ_ROUNDS  4
#define WRITE_ROUNDS 4

static int
append_string(char *buffer, int offset, const char *string)
{
  while (*string)
    buffer[offset++] = *string++;
  return offset;
}

static int
append_int(char *buffer, int offset, int value)
{
  char digits[12];
  int digit_count = 0;

  if (value == 0) {
    buffer[offset++] = '0';
    return offset;
  }
  if (value < 0) {
    buffer[offset++] = '-';
    value = -value;
  }
  while (value > 0) {
    digits[digit_count++] = '0' + value % 10;
    value /= 10;
  }
  while (digit_count > 0)
    buffer[offset++] = digits[--digit_count];
  return offset;
}

static void
log_event(const char *operation, const char *phase, int value, int exclusive)
{
  char buffer[128];
  int offset = 0;

  offset = append_string(buffer, offset, "[t=");
  offset = append_int(buffer, offset, uptime());
  offset = append_string(buffer, offset, " pid=");
  offset = append_int(buffer, offset, getpid());
  offset = append_string(buffer, offset, "] ");
  offset = append_string(buffer, offset, operation);
  offset = append_string(buffer, offset, phase);
  offset = append_string(buffer, offset, " shared_data=");
  offset = append_int(buffer, offset, value);
  if (exclusive)
    offset = append_string(buffer, offset, " (exclusive)");
  buffer[offset++] = '\n';
  write(1, buffer, offset);
}

static void
reader(void)
{
  for (int round = 0; round < READ_ROUNDS; round++) {
    rw_read_enter();
    log_event("READ  ", "start", rw_get_data(), 0);
    pause(2);
    log_event("READ  ", "end  ", rw_get_data(), 0);
    rw_read_exit();
    pause(1);
  }
  exit(0);
}

static void
writer(void)
{
  for (int round = 0; round < WRITE_ROUNDS; round++) {
    rw_write_enter();
    int value = rw_increment_data();
    log_event("WRITE ", "start/end", value, 1);
    pause(1);
    rw_write_exit();
    pause(2);
  }
  exit(0);
}

int
main(void)
{
  int child_count = NREADERS + NWRITERS;

  printf("reader-writer test: 3 readers, 2 writers\n");

  for (int i = 0; i < NREADERS; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("readwrite: reader fork failed\n");
      exit(1);
    }
    if (pid == 0)
      reader();
  }

  for (int i = 0; i < NWRITERS; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("readwrite: writer fork failed\n");
      exit(1);
    }
    if (pid == 0)
      writer();
  }

  for (int i = 0; i < child_count; i++)
    wait(0);

  printf("reader-writer test complete: final shared_data=%d\n", rw_get_data());
  exit(0);
}