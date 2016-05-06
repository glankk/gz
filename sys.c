#include <sys/stat.h>
#include <sys/fcntl.h>
#define STDIN 0
#define STDOUT 1
#define STDERR 2

static int (*stdout_handler)(const char*, int);


int write(int fd, char *ptr, int size)
{
  if (fd == STDOUT) {
    if (stdout_handler)
      return stdout_handler(ptr, size);
    return size;
  }
  return -1;
}

void set_stdout(int (*handler)(const char*, int))
{
  stdout_handler = handler;
}
