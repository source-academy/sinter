#include <stdint.h>
#include <stdio.h>

#include <driver/uart.h>
#include <esp_system.h>
#include <esp_vfs_dev.h>
#include <sinter.h>

static _Noreturn void app_exit(void) {
  printf("Restarting now.\n");
  fflush(stdout);
  esp_restart();
  while (1) {
    (void)0;
  }
}

void app_main(void) {
  uart_driver_install(CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0);
  esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);
  esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_LF);
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);

  printf("Sinter ESP32 runner.\n");
  uint32_t binary_size = 0;

  if (fread(&binary_size, 1, sizeof(uint32_t), stdin) != 4) {
    printf("Failed to read binary size.\n");
    app_exit();
  }

  unsigned char *binary = malloc(binary_size);
  if (!binary) {
    printf("Failed to allocate memory for binary.\n");
    app_exit();
  }

  uint32_t read = 0;
  while (read < binary_size) {
    size_t read_now = fread(binary + read, 1, binary_size - read, stdin);
    if (read_now == 0) {
      break;
    }
    read += read_now;
  }

  if (read < binary_size) {
    printf("Failed to read binary.\n");
    app_exit();
  }

  sinter_value_t result;
  sinter_fault_t fault = sinter_run(binary, binary_size, &result);

  printf("Program exited with fault %d and result type %d (%d, %d, %f)\n", fault, result.type, result.integer_value, result.boolean_value, result.float_value);
  app_exit();
}
