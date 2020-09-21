#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#ifdef INTERRUPT_ASSIGNMENT

#include "delay.h"
#include "gpio_isr.h"
#include "lpc_peripherals.h"
#include "semphr.h"

#endif

#ifdef INTERRUPT_ASSIGNMENT

static void gpio_interrupt(void);
static void sleep_on_sem_task(void *p);

static SemaphoreHandle_t waitOnSemaphore;

#else

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

#endif

int main(void) {

#ifdef INTERRUPT_ASSIGNMENT

  waitOnSemaphore = xSemaphoreCreateBinary();

  static gpio_s switch2;
  switch2 = board_io__get_sw2();

  // Set GPIO 0 Pin 30 as a input
  gpio__set_as_input(switch2);

  gpio0__attach_interrupt(switch2.pin_number, GPIO_INTR__FALLING_EDGE, gpio_interrupt);

  // LPC_GPIOINT->IO0IntEnR |= (1 << 30);

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, "gpio0");

  NVIC_EnableIRQ(GPIO_IRQn);

  /** PART 0
    static gpio_s led2;

    led2 = board_io__get_led2();
    while (1) {
      delay__ms(100);
      gpio__toggle(led2);
    } */
  xTaskCreate(sleep_on_sem_task, "waitOnSem", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

#else
  create_blinky_tasks();
  create_uart_task();
#endif
  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

#ifdef INTERRUPT_ASSIGNMENT

static void gpio_interrupt(void) {
  fprintf(stderr, "Switch is pressed\n");
  if (!xSemaphoreGiveFromISR(waitOnSemaphore, NULL))
    fprintf(stderr, "Unable to Release Semaphore");
}

static void sleep_on_sem_task(void *p) {
  while (1) {
    if (xSemaphoreTake(waitOnSemaphore, portMAX_DELAY)) {
      static gpio_s led3;
      led3 = board_io__get_led3();
      int count = 4;
      while (count--) {
        delay__ms(100);
        gpio__toggle(led3);
      }
    }
  }
}

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