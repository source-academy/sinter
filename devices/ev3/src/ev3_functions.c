#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
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

#define EV3_BUTTON_PATH "/dev/input/by-path/platform-gpio_keys-event"
#define EV3_MOTOR_PATH "/sys/class/tacho-motor"
#define EV3_SENSOR_PATH "/sys/class/lego-sensor"
#define EV3_OUT(letter) ("ev3-ports:out" letter)
#define EV3_IN(letter) ("ev3-ports:in" letter)

#define EV3_LED_LEFT_GREEN "/sys/class/leds/led0:green:brick-status/brightness"
#define EV3_LED_LEFT_RED "/sys/class/leds/led0:red:brick-status/brightness"
#define EV3_LED_RIGHT_GREEN "/sys/class/leds/led1:green:brick-status/brightness"
#define EV3_LED_RIGHT_RED "/sys/class/leds/led1:red:brick-status/brightness"

static char *ev3_led_paths[] = {EV3_LED_LEFT_GREEN, EV3_LED_LEFT_RED, EV3_LED_RIGHT_GREEN,
                                EV3_LED_RIGHT_RED};

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

// Makes the path to the file of the sensor of the given index in ev3_path_buf
static void make_sensor_path(int index, const char *const file) {
  snprintf(ev3_path_buf, sizeof(ev3_path_buf), EV3_SENSOR_PATH "/sensor%d%s%s", index,
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

static void reset_ev3(void) {
  // kill motors
  DIR *dir = opendir(EV3_MOTOR_PATH);
  if (!dir) {
    return;
  }

  struct dirent *dirent = NULL;
  while ((dirent = readdir(dir))) {
    if (snprintf(ev3_path_buf, sizeof(ev3_path_buf), "%s/%s/%s", EV3_MOTOR_PATH, dirent->d_name,
                 "command") >= (int)sizeof(ev3_path_buf)) {
      continue;
    }
    put_path("reset");
  }

  if (dir) {
    closedir(dir);
  }

  // reset LEDs
  strcpy(ev3_path_buf, EV3_LED_LEFT_GREEN);
  put_path("255");
  strcpy(ev3_path_buf, EV3_LED_LEFT_RED);
  put_path("0");
  strcpy(ev3_path_buf, EV3_LED_RIGHT_GREEN);
  put_path("255");
  strcpy(ev3_path_buf, EV3_LED_RIGHT_RED);
  put_path("0");
}

static int find_peripheral(const char *const sysfs_dir, const char *const sysfs_file,
                           const char *const match) {
  const size_t match_len = strlen(match);

  DIR *dir = opendir(sysfs_dir);
  if (!dir) {
    return -1;
  }

  struct dirent *dirent = NULL;
  while ((dirent = readdir(dir))) {
    if (snprintf(ev3_path_buf, sizeof(ev3_path_buf), "%s/%s", dirent->d_name, sysfs_file) >=
        (int)sizeof(ev3_path_buf)) {
      break;
    }
    int addressfd = openat(dirfd(dir), ev3_path_buf, O_RDONLY);
    if (addressfd == -1) {
      continue;
    }
    int readv = read(addressfd, ev3_path_buf, sizeof(ev3_path_buf));
    close(addressfd);
    if (readv >= (int)match_len && memcmp(ev3_path_buf, match, match_len) == 0) {
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
  int index = find_peripheral(EV3_MOTOR_PATH, "address", address);
  return index < 0 ? NANBOX_OFUNDEF() : NANBOX_OFINT(EV3_P_ID(ev3_p_motor, index));
}

static sinanbox_t find_touch_sensor(const char *address) {
  int index = find_peripheral(EV3_SENSOR_PATH, "address", address);
  return index < 0 ? NANBOX_OFUNDEF() : NANBOX_OFINT(EV3_P_ID(ev3_p_touch, index));
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

static sinanbox_t ev3_colorSensor(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  int index = find_peripheral(EV3_SENSOR_PATH, "driver_name", "lego-ev3-color");
  return index < 0 ? NANBOX_OFUNDEF() : NANBOX_OFINT(EV3_P_ID(ev3_p_color, index));
}

static sinanbox_t ev3_ultrasonicSensor(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  int index = find_peripheral(EV3_SENSOR_PATH, "driver_name", "lego-ev3-us");
  return index < 0 ? NANBOX_OFUNDEF() : NANBOX_OFINT(EV3_P_ID(ev3_p_ultrasonic, index));
}

static sinanbox_t ev3_gyroSensor(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  int index = find_peripheral(EV3_SENSOR_PATH, "driver_name", "lego-ev3-gyro");
  return index < 0 ? NANBOX_OFUNDEF() : NANBOX_OFINT(EV3_P_ID(ev3_p_gyro, index));
}

static sinanbox_t ev3_touchSensor1(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return find_touch_sensor(EV3_IN("1"));
}

static sinanbox_t ev3_touchSensor2(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return find_touch_sensor(EV3_IN("2"));
}

static sinanbox_t ev3_touchSensor3(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return find_touch_sensor(EV3_IN("3"));
}

static sinanbox_t ev3_touchSensor4(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return find_touch_sensor(EV3_IN("4"));
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
  case ev3_p_touch:
    make_sensor_path(index, NULL);
    return NANBOX_OFBOOL(file_exists(ev3_path_buf));
  case ev3_p_gyro:
    make_sensor_path(index, NULL);
    return NANBOX_OFBOOL(file_exists(ev3_path_buf));
  case ev3_p_color:
    make_sensor_path(index, NULL);
    return NANBOX_OFBOOL(file_exists(ev3_path_buf));
  case ev3_p_ultrasonic:
    make_sensor_path(index, NULL);
    return NANBOX_OFBOOL(file_exists(ev3_path_buf));
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

// ev3_motorSetStopAction(motor, stopAction)
static sinanbox_t ev3_motorSetStopAction(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  CHECK_P_ID_NOFAULT(argv[0]);

  siheap_header_t *p = SIHEAP_NANBOXTOPTR(argv[1]);
  if (NANBOX_ISPTR(argv[1]) && siheap_is_string(p)) {
    int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
    const char *s = sistrobj_tocharptr(p);
    make_motor_path(indx, "stop_action");
    printf_path("%s", s);
  }
  return NANBOX_OFUNDEF();
}

// ev3_motorGetPosition(motor)
static sinanbox_t ev3_motorGetPosition(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_motor_path(indx, "position");

  int position = 0;
  if (scanf_path("%d", &position) == 1) {
    return NANBOX_WRAP_INT(position);
  }

  return NANBOX_OFUNDEF();
}

// ev3_runForTime(motor, time, speed)
static sinanbox_t ev3_runForTime(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(3);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));

  if (NANBOX_ISNUMERIC(argv[1]) && NANBOX_ISNUMERIC(argv[2])) {
    make_motor_path(indx, "time_sp");
    printf_path("%d", nanbox_toi32(argv[1]));
    make_motor_path(indx, "speed_sp");
    printf_path("%d", nanbox_toi32(argv[2]));
    make_motor_path(indx, "command");
    put_path("run-timed");
  }

  return NANBOX_OFUNDEF();
}

// ev3_runToAbsolutePosition(motor, position, speed)
static sinanbox_t ev3_runToAbsolutePosition(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(3);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));

  if (NANBOX_ISNUMERIC(argv[1]) && NANBOX_ISNUMERIC(argv[2])) {
    make_motor_path(indx, "position_sp");
    printf_path("%d", nanbox_toi32(argv[1]));
    make_motor_path(indx, "speed_sp");
    printf_path("%d", nanbox_toi32(argv[2]));
    make_motor_path(indx, "command");
    put_path("run-to-abs-pos");
  }

  return NANBOX_OFUNDEF();
}

// ev3_runToRelativePosition(motor, position, speed)
static sinanbox_t ev3_runToRelativePosition(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(3);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));

  if (NANBOX_ISNUMERIC(argv[1]) && NANBOX_ISNUMERIC(argv[2])) {
    make_motor_path(indx, "position_sp");
    printf_path("%d", nanbox_toi32(argv[1]));
    make_motor_path(indx, "speed_sp");
    printf_path("%d", nanbox_toi32(argv[2]));
    make_motor_path(indx, "command");
    put_path("run-to-rel-pos");
  }

  return NANBOX_OFUNDEF();
}

// ev3_colorSensorRed(colorSensor)
static sinanbox_t ev3_colorSensorRed(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("RGB-RAW");

  make_sensor_path(indx, "value0");
  int red = 0;
  if (scanf_path("%d", &red) == 1) {
    return NANBOX_WRAP_INT(red);
  }

  return NANBOX_OFUNDEF();
}

// ev3_colorSensorGreen(colorSensor)
static sinanbox_t ev3_colorSensorGreen(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("RGB-RAW");

  make_sensor_path(indx, "value1");
  int green = 0;
  if (scanf_path("%d", &green) == 1) {
    return NANBOX_WRAP_INT(green);
  }

  return NANBOX_OFUNDEF();
}

// ev3_colorSensorBlue(colorSensor)
static sinanbox_t ev3_colorSensorBlue(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("RGB-RAW");

  make_sensor_path(indx, "value2");
  int blue = 0;
  if (scanf_path("%d", &blue) == 1) {
    return NANBOX_WRAP_INT(blue);
  }

  return NANBOX_OFUNDEF();
}

// ev3_reflectedLightIntensity(colorSensor)
static sinanbox_t ev3_reflectedLightIntensity(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("COL-REFLECT");

  make_sensor_path(indx, "value0");
  int reflect = 0;
  if (scanf_path("%d", &reflect) == 1) {
    return NANBOX_WRAP_INT(reflect);
  }

  return NANBOX_OFUNDEF();
}

// ev3_ambientLightIntensity(colorSensor)
static sinanbox_t ev3_ambientLightIntensity(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("COL-AMBIENT");

  make_sensor_path(indx, "value0");
  int ambient = 0;
  if (scanf_path("%d", &ambient) == 1) {
    return NANBOX_WRAP_INT(ambient);
  }

  return NANBOX_OFUNDEF();
}

// ev3_colorSensorGetColor(colorSensor)
static sinanbox_t ev3_colorSensorGetColor(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("COL-COLOR");

  make_sensor_path(indx, "value0");
  int color = 0;
  if (scanf_path("%d", &color) == 1) {
    return NANBOX_WRAP_INT(color);
  }

  return NANBOX_OFUNDEF();
}

// ev3_colorSensorGetColor(ultrasonicSensor)
static sinanbox_t ev3_ultrasonicSensorDistance(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("US-DIST-CM");

  make_sensor_path(indx, "value0");
  int distance = 0;
  if (scanf_path("%d", &distance) == 1) {
    return NANBOX_WRAP_INT(distance);
  }

  return NANBOX_OFUNDEF();
}

// ev3_gyroSensorAngle(gyroSensor)
static sinanbox_t ev3_gyroSensorAngle(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("GYRO-ANG");

  make_sensor_path(indx, "value0");
  int angle = 0;
  if (scanf_path("%d", &angle) == 1) {
    return NANBOX_WRAP_INT(angle);
  }

  return NANBOX_OFUNDEF();
}

// ev3_gyroSensorRate(gyroSensor)
static sinanbox_t ev3_gyroSensorRate(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("GYRO-RATE");

  make_sensor_path(indx, "value0");
  int rate = 0;
  if (scanf_path("%d", &rate) == 1) {
    return NANBOX_WRAP_INT(rate);
  }

  return NANBOX_OFUNDEF();
}

// ev3_touchSensorPressed(touchSensor)
static sinanbox_t ev3_touchSensorPressed(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  int indx = EV3_P_INDEX(NANBOX_INT(argv[0]));
  make_sensor_path(indx, "mode");
  put_path("TOUCH");

  make_sensor_path(indx, "value0");
  int pressed = 0;
  if (scanf_path("%d", &pressed) == 1) {
    return NANBOX_OFBOOL(pressed);
  }

  return NANBOX_OFUNDEF();
}

static svm_constant_t *hello_world_string = NULL;

// ev3_hello()
static sinanbox_t ev3_hello(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  siheap_strconst_t *str = sistrconst_new(hello_world_string);
  return SIHEAP_PTRTONANBOX(str);
}

// ev3_waitForButtonPress()
static sinanbox_t ev3_waitForButtonPress(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  struct input_event e;
  int ifd = open(EV3_BUTTON_PATH, O_RDONLY);
  if (ifd < 0) {
    goto fail;
  }

  while (true) {
    if (read(ifd, &e, sizeof(e)) == -1) {
      goto fail;
    }

    if (e.type != EV_KEY || e.value != 1) {
      continue;
    }

    switch (e.code) {
    case KEY_ENTER:
      return NANBOX_OFINT(0);
    case KEY_BACKSPACE:
      return NANBOX_OFINT(1);
    case KEY_LEFT:
      return NANBOX_OFINT(2);
    case KEY_RIGHT:
      return NANBOX_OFINT(3);
    case KEY_UP:
      return NANBOX_OFINT(4);
    case KEY_DOWN:
      return NANBOX_OFINT(5);
    }
  }

fail:
  if (ifd >= 0) {
    close(ifd);
  }
  return NANBOX_OFUNDEF();
}

// ev3_speak(words)
static sinanbox_t ev3_speak(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  siheap_header_t *wordsp = SIHEAP_NANBOXTOPTR(argv[0]);
  const char *words = NULL;
  if (NANBOX_ISPTR(argv[0]) && siheap_is_string(wordsp)) {
    words = sistrobj_tocharptr(wordsp);
  } else {
    return NANBOX_OFUNDEF();
  }

  int fd[4] = {-1, -1, -1, -1};
  pid_t c[2] = {-1, -1};
  if (pipe2(fd, O_CLOEXEC) == -1) {
    goto fail;
  }
  if (pipe2(fd + 2, O_CLOEXEC) == -1) {
    goto fail;
  }

  c[0] = fork();
  if (c[0] == 0) {
    dup2(fd[0], 0);
    dup2(fd[3], 1);
    if (execl("/usr/bin/espeak", "/usr/bin/espeak", "--stdin", "--stdout", NULL) == -1) {
      perror("execl espeak");
    }
    _Exit(1);
  } else if (c[0] == -1) {
    goto fail;
  }

  c[1] = fork();
  if (c[1] == 0) {
    dup2(fd[2], 0);
    if (execl("/usr/bin/aplay", "/usr/bin/aplay", "-q", NULL) == -1) {
      perror("execl aplay");
    }
    _Exit(1);
  } else if (c[1] == -1) {
    goto fail;
  }

  const ssize_t wordslen = strlen(words);
  ssize_t written = 0;
  while (written < wordslen) {
    ssize_t this_written = write(fd[1], words + written, wordslen - written);
    if (this_written == -1) {
      goto fail;
    }
    written += this_written;
  }

fail:
  for (unsigned int i = 0; i < sizeof(fd) / sizeof(*fd); ++i) {
    if (fd[i] >= 3) {
      close(fd[i]);
    }
  }
  for (unsigned int i = 0; i < sizeof(c) / sizeof(*c); ++i) {
    if (c[i] > 0) {
      waitpid(c[i], NULL, 0);
    }
  }
  return NANBOX_OFUNDEF();
}

// ev3_playSequence([frequency, length (ms), delay (ms), ...])
static sinanbox_t ev3_playSequence(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  pid_t beep_pid = fork();
  if (beep_pid != 0) {
    if (beep_pid > 0) {
      waitpid(beep_pid, NULL, 0);
    }
    return NANBOX_OFUNDEF();
  }

  siheap_header_t *arrayp = SIHEAP_NANBOXTOPTR(argv[0]);
  siheap_array_t *array = (siheap_array_t *)arrayp;
  if (!NANBOX_ISPTR(argv[0]) || arrayp->type != sitype_array) {
    _Exit(1);
  }

  const size_t num_beeps = array->count / 3;
  if (!num_beeps) {
    _Exit(1);
  }
  char **exec_argv = calloc(num_beeps * 7 + 1, sizeof(char *));
  if (!exec_argv) {
    _Exit(1);
  }

  for (size_t i = 0; i < num_beeps; ++i) {
    const size_t arrayv_base = i * 3, argv_base = i * 7;
    sinanbox_t freqv = siarray_get(array, arrayv_base),
               lengthv = siarray_get(array, arrayv_base + 1),
               delayv = siarray_get(array, arrayv_base + 2);
    if (!NANBOX_ISNUMERIC(freqv) || !NANBOX_ISNUMERIC(lengthv) || !NANBOX_ISNUMERIC(delayv)) {
      _Exit(1);
    }

    exec_argv[argv_base + 0] = "-n";
    exec_argv[argv_base + 1] = "-f";
    exec_argv[argv_base + 3] = "-l";
    exec_argv[argv_base + 5] = "-D";

    if (asprintf(&exec_argv[argv_base + 2], "%" PRIu32, nanbox_tou32(freqv)) == -1 ||
        asprintf(&exec_argv[argv_base + 4], "%" PRIu32, nanbox_tou32(lengthv)) == -1 ||
        asprintf(&exec_argv[argv_base + 6], "%" PRIu32, nanbox_tou32(delayv)) == -1) {
      _Exit(1);
    }
  }

  exec_argv[0] = "/usr/bin/beep";
  // don't bother cleaning up, we're gonna exec anyway!
  execv(exec_argv[0], exec_argv);
  _Exit(1);
}

static sinanbox_t ev3_ledLeftGreen(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return NANBOX_OFINT(0);
}

static sinanbox_t ev3_ledLeftRed(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return NANBOX_OFINT(1);
}

static sinanbox_t ev3_ledRightGreen(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return NANBOX_OFINT(2);
}

static sinanbox_t ev3_ledRightRed(uint8_t argc, sinanbox_t *argv) {
  ARGS_UNUSED;
  return NANBOX_OFINT(3);
}

// ev3_ledGetBrightness(led)
static sinanbox_t ev3_ledGetBrightness(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  CHECK_P_ID_NOFAULT(argv[0]);

  unsigned int ledNumber = (unsigned int)NANBOX_INT(argv[0]);
  if (ledNumber >= sizeof(ev3_led_paths) / sizeof(*ev3_led_paths)) {
    return NANBOX_OFUNDEF();
  }

  strcpy(ev3_path_buf, ev3_led_paths[ledNumber]);

  int brightness = -1;
  if (scanf_path("%d", &brightness) == 1) {
    return NANBOX_WRAP_INT(brightness);
  }
  return NANBOX_OFUNDEF();
}

// ev3_ledGetBrightness(led, brightness)
static sinanbox_t ev3_ledSetBrightness(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  CHECK_P_ID_NOFAULT(argv[0]);

  unsigned int ledNumber = (unsigned int)NANBOX_INT(argv[0]);
  if (ledNumber >= sizeof(ev3_led_paths) / sizeof(*ev3_led_paths) || !NANBOX_ISNUMERIC(argv[1])) {
    return NANBOX_OFUNDEF();
  }

  strcpy(ev3_path_buf, ev3_led_paths[ledNumber]);

  int brightness = nanbox_tou32(argv[1]);
  if (brightness > 255) {
    brightness = 255;
  } else if (brightness < 0) {
    brightness = 0;
  }
  printf_path("%d", brightness);
  return NANBOX_OFUNDEF();
}

static const sivmfnptr_t internals[] = {ev3_pause,
                                        ev3_connected,
                                        ev3_motorA,
                                        ev3_motorB,
                                        ev3_motorC,
                                        ev3_motorD,
                                        ev3_motorGetSpeed,
                                        ev3_motorSetSpeed,
                                        ev3_motorStart,
                                        ev3_motorStop,
                                        ev3_motorSetStopAction,
                                        ev3_motorGetPosition,
                                        ev3_runForTime,
                                        ev3_runToAbsolutePosition,
                                        ev3_runToRelativePosition,
                                        ev3_colorSensor,
                                        ev3_colorSensorRed,
                                        ev3_colorSensorGreen,
                                        ev3_colorSensorBlue,
                                        ev3_reflectedLightIntensity,
                                        ev3_ambientLightIntensity,
                                        ev3_colorSensorGetColor,
                                        ev3_ultrasonicSensor,
                                        ev3_ultrasonicSensorDistance,
                                        ev3_gyroSensor,
                                        ev3_gyroSensorAngle,
                                        ev3_gyroSensorRate,
                                        ev3_touchSensor1,
                                        ev3_touchSensor2,
                                        ev3_touchSensor3,
                                        ev3_touchSensor4,
                                        ev3_touchSensorPressed,
                                        ev3_hello,
                                        ev3_waitForButtonPress,
                                        ev3_speak,
                                        ev3_playSequence,
                                        ev3_ledLeftGreen,
                                        ev3_ledLeftRed,
                                        ev3_ledRightGreen,
                                        ev3_ledRightRed,
                                        ev3_ledGetBrightness,
                                        ev3_ledSetBrightness};
static const size_t internals_count = sizeof(internals) / sizeof(*internals);

void setup_internals(void) {
  atexit(reset_ev3);

  static const char *hello_world_cstr = "Hello there! Welcome to LEGO EV3 (adapted by CS1101S)!";
  hello_world_string = malloc(sizeof(svm_constant_t) + strlen(hello_world_cstr) + 1);
  hello_world_string->type = 1;
  hello_world_string->length = strlen(hello_world_cstr) + 1;
  strcpy((char *)hello_world_string->data, hello_world_cstr);

  sivmfn_vminternals = internals;
  sivmfn_vminternal_count = internals_count;
}
