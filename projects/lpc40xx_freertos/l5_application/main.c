#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "i2c.h"
#include "i2c_slave_function.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

//#define I2C_MASTER
#define I2C_SLAVE

// 'static' to make these functions 'private' to this file
static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);
static void i2c_task(void *p);
static void pin_select_for_i2c();
static void i2c_mt_task(void *p);
static void i2c_slave_task(void *p);

int main(void) {
#ifdef I2C_LAB_ASSGNMT
  const uint32_t desired_i2c_bus_speed_in_hz = 100 * (1000);
  const uint8_t i2c_slave_device_address = 0x07;
  i2c__initialize(I2C__0, desired_i2c_bus_speed_in_hz, clock__get_peripheral_clock_hz());
  pin_select_for_i2c();
#ifdef I2C_SLAVE
  i2c2__slave_init(i2c_slave_device_address);
#ifdef I2C_LAB_ASSGNMT_PART_B
  xTaskCreate(i2c_slave_task, "i2c_slave", 1024 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif
#endif
#ifdef I2C_MASTER
#ifdef I2C_LAB_ASSGNMT_PART_A
  xTaskCreate(i2c_task, "i2c", 1024 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif

#ifdef I2C_LAB_ASSGNMT_PART_B
  xTaskCreate(i2c_mt_task, "I2C_MT", 1024 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif
#endif
  // create_blinky_tasks();
  create_uart_task();
#endif
  // If you have the ESP32 wifi module soldered on the board, you can try uncommenting this code
  // See esp32/README.md for more details
  // uart3_init();                                                                     // Also include:  uart3_init.h
  // xTaskCreate(esp32_tcp_hello_world_task, "uart3", 1000, NULL, PRIORITY_LOW, NULL); // Include esp32_task.h

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

#ifdef I2C_LAB_ASSGNMT

#ifdef I2C_MASTER

#ifdef I2C_LAB_ASSGNMT_PART_A
void i2c_task(void *p) {
  while (1) {
    if (i2c__detect(I2C__0, 0x0E)) {
      printf("Successfully detected slave\n");
    } else {
      printf("Failed to detected slave\n");
    }
    vTaskDelay(1000);
  }
}
#endif

#ifdef I2C_LAB_ASSGNMT_PART_B

void i2c_mt_task(void *p) {
  const uint8_t byte_to_send = 0xA4;
  const uint8_t clear_byte_data = 0x00;
  uint8_t slave_data_read;

  while (1) {
    // Clearing the register 0, before writing register 1, as per slave requirement
    if (i2c__write_slave_data(I2C__0, 0x0E, 0, &clear_byte_data, 1)) {
      printf("Sent data %d successfully to Slave register 0\n", clear_byte_data);
    } else {
      printf("Failed to send data %d to slave register 0\n", clear_byte_data);
    }
    if (i2c__write_slave_data(I2C__0, 0x0E, 1, &byte_to_send, 1)) {
      printf("Sent data %d successfully to Slave register 1\n", byte_to_send);
    } else {
      printf("Failed to send data %d to slave register 1\n", byte_to_send);
    }
    // if (i2c__read_slave_data(I2C__0, 0x0E, 1, &slave_data_read, 1)) {
    //   if (slave_data_read == byte_to_send) {
    //     printf("Successfully read data %d \n", slave_data_read);
    //   } else {
    //     printf("Data read is %d, which is not same as written\n", slave_data_read);
    //   }
    // } else {
    //   printf("Failed to read data \n");
    // }
    vTaskDelay(1000);
  }
}
#endif

#endif

#ifdef I2C_SLAVE
#ifdef I2C_LAB_ASSGNMT_PART_B

static void i2c_slave_function_handler(uint8_t register_index, uint8_t register_data) {
  const uint8_t led0_bit_mask = (1 << 4);
  const uint8_t led1_bit_mask = (1 << 5);
  const uint8_t led2_bit_mask = (1 << 6);
  const uint8_t led3_bit_mask = (1 << 7);
  const uint8_t blink_duration_in_sec_bit_mask = (0b111);
  gpio_s led0 = board_io__get_led0();
  gpio_s led1 = board_io__get_led1();
  gpio_s led2 = board_io__get_led2();
  gpio_s led3 = board_io__get_led3();

  switch (register_index) {
  case 1:
    printf("Toggling LEDs");
    if (register_data & led0_bit_mask) {
      printf(" led0");
      gpio__toggle(led0);
    }
    if (register_data & led1_bit_mask) {
      printf(" led1");
      gpio__toggle(led1);
    }
    if (register_data & led2_bit_mask) {
      printf(" led2");
      gpio__toggle(led2);
    }
    if (register_data & led3_bit_mask) {
      printf(" led3");
      gpio__toggle(led3);
    }
    uint32_t blink_duration_in_ms = ((register_data >> 1) & blink_duration_in_sec_bit_mask) * 1000;
    printf(" with gap of %ld ms\n", blink_duration_in_ms);
    vTaskDelay(blink_duration_in_ms);
    break;

  case 0:
    (register_data & led0_bit_mask) ? gpio__reset(led0) : gpio__set(led0);
    (register_data & led1_bit_mask) ? gpio__reset(led1) : gpio__set(led1);
    (register_data & led2_bit_mask) ? gpio__reset(led2) : gpio__set(led2);
    (register_data & led3_bit_mask) ? gpio__reset(led3) : gpio__set(led3);
    break;
  }
}

void i2c_slave_task(void *p) {

  while (1) {
    uint8_t slave_memory_value;
    // LED on/off register index 0
    // i2c_slave_callback__read_memory(0, &slave_memory_value);
    // i2c_slave_function_handler(0, slave_memory_value);
    // LED blinking register index 1
    i2c_slave_callback__read_memory(1, &slave_memory_value);
    i2c_slave_function_handler(1, slave_memory_value);

    vTaskDelay(100);
  }
}

#endif
#endif

static void pin_select_for_i2c() {
  const uint32_t i2c_function_bit = 0b100;
  const uint32_t od_bit_mask = (1 << 10);
  LPC_IOCON->P1_30 |= (od_bit_mask | i2c_function_bit);
  LPC_IOCON->P1_31 |= (od_bit_mask | i2c_function_bit);
}

static void create_blinky_tasks(void) {
  /**
   * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
   * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
   */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes, !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}
#endif