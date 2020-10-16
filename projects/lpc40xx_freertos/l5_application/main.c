#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "stdlib.h"
#include "string.h"

#ifdef UART_ASSIGNMENT

#include "uart_lab.h"

#define CURRENT_UART_CHANNEL UART_2

#ifdef PART3_UART_ASSGNMT
#define BOARD_1
//#define BOARD_2
#ifdef BOARD_1
static void board_1_sender_task(void *p);
#endif
#ifdef BOARD_2
static void board_2_receiver_task(void *p);
#endif
#endif
#if defined(PART2_UART_ASSGNMT) || defined(PART1_UART_ASSGNMT)
static void uart_read_task(void *p);
static void uart_write_task(void *p);
#endif

#else

// 'static' to make these functions 'private' to this file
static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

#endif

int main(void) {

#ifdef UART_ASSIGNMENT
  const uint32_t peripheral_clock = clock__get_peripheral_clock_hz();
  const uint32_t uart_baud_rate = 115200;
  uart_lab__init(CURRENT_UART_CHANNEL, peripheral_clock, uart_baud_rate);
#if defined(PART2_UART_ASSGNMT) || defined(PART3_UART_ASSGNMT)
  uart__enable_receive_interrupt(CURRENT_UART_CHANNEL);
#endif
#if defined(PART2_UART_ASSGNMT) || defined(PART1_UART_ASSGNMT)
  xTaskCreate(uart_write_task, "UART_WRITE", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(uart_read_task, "UART_READ", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif

#if defined(PART3_UART_ASSGNMT) && defined(BOARD_1)
  xTaskCreate(board_1_sender_task, "board1_send", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif

#if defined(PART3_UART_ASSGNMT) && defined(BOARD_2)
  xTaskCreate(board_2_receiver_task, "board2_receive", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif

#else
  create_blinky_tasks();
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

#ifdef UART_ASSIGNMENT

#ifdef PART3_UART_ASSGNMT

#ifdef BOARD_1
// This task is done for you, but you should understand what this code is doing
void board_1_sender_task(void *p) {
  char number_as_string[16] = {0};

  while (true) {
    const int number = rand();
    sprintf(number_as_string, "%i", number);

    // Send one char at a time to the other board including terminating NULL char
    for (int i = 0; i <= strlen(number_as_string); i++) {
      uart_lab__polled_put(CURRENT_UART_CHANNEL, number_as_string[i]);
      printf("Sent: %c\n", number_as_string[i]);
    }

    printf("Sent: %i over UART to the other board\n", number);
    vTaskDelay(3000);
  }
}
#endif
#ifdef BOARD_2

void board_2_receiver_task(void *p) {
  char number_as_string[16] = {0};
  int counter = 0;

  while (true) {
    char byte = 0;
    uart_lab__get_char_from_queue(&byte, portMAX_DELAY);
    printf("Received: %c\n", byte);

    // This is the last char, so print the number
    if ('\0' == byte) {
      number_as_string[counter] = '\0';
      counter = 0;
      printf("Received this number from the other board: %s\n", number_as_string);
    }
    // We have not yet received the NULL '\0' char, so buffer the data
    else {
      number_as_string[counter++] = byte;
    }
  }
}

#endif

#else

void uart_read_task(void *p) {
  while (1) {
    char read_byte = 0;
#ifdef PART2_UART_ASSGNMT
    while (uart_lab__get_char_from_queue(&read_byte, 100)) {
      printf("The read data is %X\n", read_byte);
    }
#endif

#ifdef PART1_UART_ASSGNMT
    while (!(uart_lab__polled_get(CURRENT_UART_CHANNEL, &read_byte))) {
    }
    printf("The read data is %X\n", read_byte);
#endif
    vTaskDelay(50);
  }
}

void uart_write_task(void *p) {
  while (1) {
    const uint8_t write_byte = 0xAA;
    while (!(uart_lab__polled_put(CURRENT_UART_CHANNEL, write_byte))) {
    }
    printf("The data %X is written successfully\n", write_byte);
    vTaskDelay(50);
  }
}
#endif
#else

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
