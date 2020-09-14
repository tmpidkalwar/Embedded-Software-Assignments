#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

/**
 * Local Macros
 */
#define INVALID_SWITCH 4
#define MAX_LEDS 4

/**
 * Local Functions
 */
static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

/**
 * Local Variables
 */
static SemaphoreHandle_t switch_press_indication;
static volatile uint8_t led_index = 0;

static void led_task(void *task_parameter) {
  // Available LEDs served under this task
  const gpio_lab_s led[] = {{1, 18}, {1, 24}, {1, 26}, {2, 3}};
  while (true) {
    // Check every 1000ms whether semaphore is available. Other time FreeRTOS make this task sleep
    if (xSemaphoreTake(switch_press_indication, 1000)) {
      // Temperory variable to track remaining LEDs to be blinked
      int temp = MAX_LEDS;

      // This loop will turn on all LEDs starting led_index
      while (temp) {
        printf("led on index =%d \n", led_index);
        gpio_lab_set_high(led[led_index++ % 4]);
        vTaskDelay(100);
        temp--;
      }
      temp = MAX_LEDS;

      // This loop will turn off all LEDs starting led_index
      while (temp) {
        printf("led off index =%d \n", led_index);
        gpio_lab_set_low(led[led_index++ % 4]);
        vTaskDelay(100);
        temp--;
      }
    } else {
      puts("Timeout: No switch press indication for 1000ms");
    }
  }
}

static void switch_task(void *task_parameter) {
  // Available switches served under this task
  const gpio_lab_s switch_index[] = {{1, 19}, {1, 15}, {0, 30}, {0, 29}};
  // Initialize the switch to be toggle to invalid switch
  uint8_t toggle_switch = INVALID_SWITCH;

  while (true) {
    // Monitor pressed switch for successful toggle
    switch (toggle_switch) {
    case 3:
      if (!gpio_lab_get_level(switch_index[3])) {
        toggle_switch = INVALID_SWITCH;
        led_index = 0;
        // Release the semaphore when the switch is successfully toggled
        if (!xSemaphoreGive(switch_press_indication))
          puts("Unable to release the semaphore");
      }
      break;
    case 2:
      if (!gpio_lab_get_level(switch_index[2])) {
        toggle_switch = INVALID_SWITCH;
        led_index = 1;
        // Release the semaphore when the switch is successfully toggled
        if (!xSemaphoreGive(switch_press_indication))
          puts("Unable to release the semaphore");
      }
      break;
    case 1:
      if (!gpio_lab_get_level(switch_index[1])) {
        toggle_switch = INVALID_SWITCH;
        led_index = 2;
        // Release the semaphore when the switch is successfully toggled
        if (!xSemaphoreGive(switch_press_indication))
          puts("Unable to release the semaphore");
      }
      break;
    case 0:
      if (!gpio_lab_get_level(switch_index[0])) {
        toggle_switch = INVALID_SWITCH;
        led_index = 3;
        // Release the semaphore when the switch is successfully toggled
        if (!xSemaphoreGive(switch_press_indication))
          puts("Unable to release the semaphore");
      }
      break;
    default:
      break;
    }

    // Monitor the pressed switch
    if (gpio_lab_get_level(switch_index[3]))
      toggle_switch = 3;
    else if (gpio_lab_get_level(switch_index[2]))
      toggle_switch = 2;
    else if (gpio_lab_get_level(switch_index[1]))
      toggle_switch = 1;
    else if (gpio_lab_get_level(switch_index[0]))
      toggle_switch = 0;
    // Task should always sleep otherwise they will use 100% CPU
    // This task sleep also helps avoid spurious semaphore give during switch debeounce
    vTaskDelay(100);
  }
}

int main(void) {

  switch_press_indication = xSemaphoreCreateBinary();

  xTaskCreate(led_task, "led", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(switch_task, "switch", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
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