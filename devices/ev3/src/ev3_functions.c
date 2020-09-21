#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <sinter.h>
#include <sinter/display.h>
#include <sinter/heap_obj.h>
#include <sinter/program.h>
#include <sinter/vm.h>

// For more information on the sysfs interface,
// - https://docs.ev3dev.org/projects/lego-linux-drivers/en/ev3dev-stretch/index.html
// - https://docs.ev3dev.org/projects/lego-linux-drivers/en/ev3dev-stretch/ev3.html
// This interface is written for the driver included in ev3dev-stretch, version 2.3.5.
// (or at least, when this comment was last updated..)

#define EV3_MOTOR_PATH "/sys/class/tacho-motor"
#define EV3_SENSOR_PATH "/sys/class/lego-sensor"
#define EV3_OUT(letter) ("ev3-ports:out" letter)
#define EV3_IN(letter) ("ev3-ports:in" letter)

// EV3 peripheral types
enum ev3_p_type {
  ev3_p_motor = 0,
  ev3_p_touch = 1,
  ev3_p_gyro = 2,
  ev3_p_color = 3,
  ev3_p_ultrasonic = 4
};

// We encode the peripheral as a 19-bit integer, with the high 3 bits the type
// and the low 16 bits the device index (as in the sysfs path)
// (note: our nanbox int is 21-bit)
#define EV3_P_ID(type, indx) ((((type)&7) << 16) | ((indx)&0xFFFFu))
#define EV3_P_TYPE(id) ((id) >> 16)
#define EV3_P_INDEX(id) ((id)&0xFFFFu)

// buffer to hold snprintf results
// assume any call to a function in this file might alter this path
// (i.e. so just snprintf and then pass it to fopen() immediately)
// note: this is safe because sinter is single-threaded
static char ev3_path_buf[128];
// examples of paths:
// /sys/class/tacho-motor/motor8/duty_cycle
// 128 should be more than enough

static bool file_exists(const char *const path) { return access(path, F_OK) == 0; }

// Makes the path to the file of the motor of the given index in ev3_path_buf
static void make_motor_path(int index, const char *const file) {
  snprintf(ev3_path_buf, sizeof(ev3_path_buf), EV3_MOTOR_PATH "/motor%d%s%s", index,
           file && *file ? "/" : "", file ? file : "");
}

// fprintfs to the path in ev3_path_buf
__attribute__((format(printf, 1, 2))) static void printf_path(const char *format, ...) {
  va_list args;
  va_start(args, format);
  FILE *f = fopen(ev3_path_buf, "w");
  if (f) {
    vfprintf(f, format, args);
    fclose(f);
  }
  va_end(args);
}

static void put_path(const char *string) {
  FILE *f = fopen(ev3_path_buf, "w");
  if (f) {
    fwrite(string, 1, strlen(string), f);
    fclose(f);
  }
}

// fscanfs from the path in ev3_path_buf
// returns the return result of vfscanf
__attribute__((format(scanf, 1, 2))) static int scanf_path(const char *format, ...) {
  int retv = -1;
  va_list args;
  va_start(args, format);
  FILE *f = fopen(ev3_path_buf, "r");
  if (f) {
    retv = vfscanf(f, format, args);
    fclose(f);
  }
  va_end(args);
  return retv;
}

static int find_peripheral(const char *const sysfs_dir, const char *const address) {
  const size_t address_len = strlen(address);

  DIR *dir = opendir(sysfs_dir);
  if (!dir) {
    return -1;
  }

  struct dirent *dirent = NULL;
  while ((dirent = readdir(dir))) {
    if (snprintf(ev3_path_buf, sizeof(ev3_path_buf), "%s/address", dirent->d_name) >=
        (int)sizeof(ev3_path_buf)) {
      break;
    }
    int addressfd = openat(dirfd(dir), ev3_path_buf, O_RDONLY);
    if (addressfd == -1) {
      continue;
    }
    int readv = read(addressfd, ev3_path_buf, sizeof(ev3_path_buf));
    close(addressfd);
    if (readv >= (int)address_len && memcmp(ev3_path_buf, address, address_len) == 0) {
      const char *device_index = dirent->d_name;
      // dirname is "sensorNNN" or "motorNNN"
      // advance to the index at the end
      while (!isdigit(*device_index) && *device_index) {
        ++device_index;
      }
      int index = atoi(device_index);
      closedir(dir);
      return index;
    }
  }

  if (dir) {
    closedir(dir);
  }
  return -1;
}

// Check that argc is at least n
#define CHECK_ARGC(n)                                                                              \
  do {                                                                                             \
    if (argc < (n)) {                                                                              \
      sifault(sinter_fault_function_arity);                                                        \
      return NANBOX_OFEMPTY();                                                                     \
    }                                                                                              \
  } while (0)

// Check that the given argument is a nanbox int (which all peripheral IDs must be), or fault
#define CHECK_P_ID(arg)                                                                            \
  do {                                                                                             \
    if (!NANBOX_ISINT(arg)) {                                                                      \
      sifault(sinter_fault_type);                                                                  \
      return NANBOX_OFEMPTY();                                                                     \
    }                                                                                              \
  } while (0)

// Check that the given argument is a nanbox int (which all peripheral IDs must be),
// but just return undefined (instead of faulting) if not
#define CHECK_P_ID_NOFAULT(arg)                                                                    \
  do {                                                                                             \
    if (!NANBOX_ISINT(arg)) {                                                                      \
      return NANBOX_OFUNDEF();                                                                     \
    }                                                                                              \
  } while (0)

#define ARGS_UNUSED                                                                                \
  (void)argc;                                                                                      \
  (void)argv

static sinanbox_t find_motor(const char *address) {
  int index = find_peripheral(EV3_MOTOR_PATH, address);
  return index < 0 ? NANBOX_OFUNDEF() : NANBOX_OFINT(EV3_P_ID(ev3_p_motor, index));
}

// ev3_pause(ms)
static sinanbox_t ev3_pause(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);

  if (NANBOX_ISINT(argv[0])) {
    int32_t ms = NANBOX_INT(argv[0]);
    nanosleep(&(struct timespec){.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000}, NULL);
  } else if (NANBOX_ISFLOAT(argv[0])) {
    float ms = NANBOX_FLOAT(argv[0]);
    nanosleep(&(struct timespec){.tv_sec = (time_t)(ms / 1000.0),
                                 .tv_nsec = (long)(fmodf(ms, 1000.0) * 1000000.0)},
              NULL);
  }

  return NANBOX_OFUNDEF();
}

static sinanbox_t ev3_motorA(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return find_motor(EV3_OUT("A"));
}

static sinanbox_t ev3_motorB(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return find_motor(EV3_OUT("B"));
}

static sinanbox_t ev3_motorC(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return find_motor(EV3_OUT("C"));
}

static sinanbox_t ev3_motorD(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return find_motor(EV3_OUT("D"));
}

// ev3_connected(peripheral)
static sinanbox_t ev3_connected(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  if (NANBOX_ISUNDEF(argv[0])) {
    return NANBOX_OFBOOL(false);
  }
  CHECK_P_ID(argv[0]);

  const int id = NANBOX_INT(argv[0]);
  const int index = EV3_P_INDEX(id);
  switch (EV3_P_TYPE(id)) {
  case ev3_p_motor:
    make_motor_path(index, NULL);
    return NANBOX_OFBOOL(file_exists(ev3_path_buf));
    break;
  case ev3_p_touch:
  case ev3_p_gyro:
  case ev3_p_color:
  case ev3_p_ultrasonic:
    // TODO
    return NANBOX_OFBOOL(false);
  }

  return NANBOX_OFBOOL(false);
}

// ev3_motorGetSpeed(motor)
static sinanbox_t ev3_motorGetSpeed(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_motor_path(indx, "speed");

  int speed = 0;
  if (scanf_path("%d", &speed) == 1) {
    return NANBOX_WRAP_INT(speed);
  }

  return NANBOX_OFUNDEF();
}

// ev3_motorSetSpeed(motor, speed)
static sinanbox_t ev3_motorSetSpeed(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  CHECK_P_ID_NOFAULT(argv[0]);

  if (NANBOX_ISNUMERIC(argv[1])) {
    int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
    make_motor_path(indx, "speed_sp");
    printf_path("%d", nanbox_toi32(argv[1]));
  }

  return NANBOX_OFUNDEF();
}

// ev3_motorStart(motor)
static sinanbox_t ev3_motorStart(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_motor_path(indx, "command");
  put_path("run-forever");

  return NANBOX_OFUNDEF();
}

// ev3_motorStop(motor)
static sinanbox_t ev3_motorStop(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_motor_path(indx, "command");
  put_path("stop");

  return NANBOX_OFUNDEF();
}

static const sivmfnptr_t internals[] = {
    ev3_pause,  ev3_connected,     ev3_motorA,        ev3_motorB,     ev3_motorC,
    ev3_motorD, ev3_motorGetSpeed, ev3_motorSetSpeed, ev3_motorStart, ev3_motorStop};
static const size_t internals_count = sizeof(internals) / sizeof(*internals);

void setup_internals(void) {
  sivmfn_vminternals = internals;
  sivmfn_vminternal_count = internals_count;
}
