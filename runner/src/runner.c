#include <stdio.h>
#include <stdint.h>
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

static void print_string(const char *s, bool is_error) {
  (void) is_error;
  printf("%s", s);
}

static void print_integer(int32_t v, bool is_error) {
  (void) is_error;
  printf("%d", v);
}

static void print_float(float v, bool is_error) {
  (void) is_error;
  printf("%f", v);
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

  sinter_printer_float = print_float;
  sinter_printer_string = print_string;
  sinter_printer_integer = print_integer;

  sinter_value_t result = { 0 };
  sinter_fault_t fault = sinter_run(program, size, &result);

  printf("Program exited with fault %d and result type %d (%d, %d, %f)\n", fault, result.type, result.integer_value, result.boolean_value, result.float_value);

  return 0;
}
