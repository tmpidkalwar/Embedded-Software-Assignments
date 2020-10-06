#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#ifdef SPI_ASSIGNMENT

#include "adesto_flash.h"
#include "semphr.h"
#include "ssp2_lab.h"

#ifdef PART1_SPI_ASSGNMT
void spi_task(void *p);
#endif

#ifdef PART2_SPI_ASSGNMT
void spi_id_verification_task(void *p);
SemaphoreHandle_t spi_bus_mutex;
#endif

#ifdef PART3_SPI_ASSGNMT
SemaphoreHandle_t write_read_page_mutex;

void flash_page_read_task(void *p);
void flash_page_write_task(void *p);
#endif

#else

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

#endif

int main(void) {
#ifdef SPI_ASSIGNMENT
  // Initialize the SPI clock with given frequency
  const uint32_t spi_clock_mhz = 6;
  ssp2_lab_init(spi_clock_mhz);

  // Configure the SPI pins
  ssp2_configure_pin_functions();

#ifdef PART1_SPI_ASSGNMT
  xTaskCreate(spi_task, "spi", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif

#ifdef PART2_SPI_ASSGNMT
  spi_bus_mutex = xSemaphoreCreateMutex();

  xTaskCreate(spi_id_verification_task, "id_verify", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(spi_id_verification_task, "id_verify1", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif

#ifdef PART3_SPI_ASSGNMT

  write_read_page_mutex = xSemaphoreCreateMutex();
  xTaskCreate(flash_page_write_task, "page_write", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(flash_page_read_task, "page_read", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif
#else
  create_blinky_tasks();
  create_uart_task();
#endif
  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

#ifdef SPI_ASSIGNMENT

#ifdef PART1_SPI_ASSGNMT

void spi_task(void *p) {
  while (1) {
    adesto_flash_id_s id = adesto_read_signature();
    fprintf(stderr, " Manufacturing id is: %X, %X, %X, %X\n", id.manufacturer_id, id.device_id_1, id.device_id_2,
            id.extended_device_id);

    vTaskDelay(2000);
  }
}
#endif

#ifdef PART2_SPI_ASSGNMT
void spi_id_verification_task(void *p) {
  while (1) {
    adesto_flash_id_s id = {0};
    if (xSemaphoreTake(spi_bus_mutex, 1000)) {
      id = adesto_read_signature();
      if (id.manufacturer_id != 0x1f) {
        // When we read a manufacturer ID we do not expect, we will kill this task if (id.manufacturer_id != 0x1F) {
        fprintf(stderr, "Manufacturer ID read failure\n");
        vTaskSuspend(NULL); // Kill this task
      }
      fprintf(stderr, "Task read manufacturing id as %x\n", id.manufacturer_id);
      xSemaphoreGive(spi_bus_mutex);
    }
  }
}
#endif

#ifdef PART3_SPI_ASSGNMT
void flash_page_read_task(void *p) {
  while (1) {
    if (xSemaphoreTake(write_read_page_mutex, 1000)) {
      adesto_flash_page_read();
      xSemaphoreGive(write_read_page_mutex);
    }
    vTaskDelay(500);
  }
}

void flash_page_write_task(void *p) {
  while (1) {
    if (xSemaphoreTake(write_read_page_mutex, 1000)) {
      adesto_flash_page_write();
      xSemaphoreGive(write_read_page_mutex);
    }
    vTaskDelay(500);
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
