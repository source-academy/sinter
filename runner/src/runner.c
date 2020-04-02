#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <sinter.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

ssize_t check_posix(ssize_t result, const char *msg) {
  if (result == -1 && errno) {
    perror(msg);
    _exit(1);
  }

  return result;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    eprintf("Usage: %s <program>\n", argv[0]);
    return 1;
  }

  int program_fd = check_posix(open(argv[1], O_RDONLY), "Failed to open program");
  off_t size;
  {
    struct stat stat_buf;
    check_posix(fstat(program_fd, &stat_buf), "fstat failed");
    size = stat_buf.st_size;
  }
  const unsigned char *program = mmap(NULL, size, PROT_READ, MAP_SHARED, program_fd, 0);
  if (program == MAP_FAILED) {
    check_posix(-1, "mmap failed");
  }

  sinter_value_t result;
  sinter_fault_t fault = sinter_run(program, size, &result);

  printf("Program exited with fault %d and result type %d (%d, %d, %f)\n", fault, result.type, result.integer_value, result.boolean_value, result.float_value);

  return 0;
}
