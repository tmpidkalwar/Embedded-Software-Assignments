#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#ifdef SPI_ASSIGNMENT

#include "ssp2_lab.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

// TODO: Implement Adesto flash memory CS signal as a GPIO driver
void adesto_cs(void) { LPC_GPIO1->CLR = (1 << 10); }
void adesto_ds(void) { LPC_GPIO1->SET = (1 << 10); }

// TODO: Study the Adesto flash 'Manufacturer and Device ID' section
typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_1;
  uint8_t device_id_2;
  uint8_t extended_device_id;
} adesto_flash_id_s;

// TODO: Implement the code to read Adesto flash memory signature
// TODO: Create struct of type 'adesto_flash_id_s' and return it
adesto_flash_id_s adesto_read_signature(void) {
  adesto_flash_id_s data = {0};
  const uint8_t dummy_byte = 0xFF;
  const uint8_t flash_id_read_opcode = 0x9F;
  adesto_cs();
  {
    // Send the opcode to trigger manufacturing ID read from flash
    ssp2_lab_exchange_byte(flash_id_read_opcode);
    // read out the manufacturing id from Flash by sending dummy bytes
    // ssp2_lab_exchange_byte(dummy_byte);
    data.manufacturer_id = ssp2_lab_exchange_byte(dummy_byte);
    // ssp2_lab_exchange_byte(dummy_byte);
    data.device_id_1 = ssp2_lab_exchange_byte(dummy_byte);
    // ssp2_lab_exchange_byte(dummy_byte);
    data.device_id_2 = ssp2_lab_exchange_byte(dummy_byte);
    // ssp2_lab_exchange_byte(dummy_byte);
    data.extended_device_id = ssp2_lab_exchange_byte(dummy_byte);
  }
  adesto_ds();

  return data;
}

#ifdef PART1_SPI_ASSGNMT

void spi_task(void *p) {
  // From the LPC schematics pdf, find the pin numbers connected to flash memory
  // Read table 84 from LPC User Manual and configure PIN functions for SPI2 pins
  // You can use gpio__construct_with_function() API from gpio.h
  //
  // Note: Configure only SCK2, MOSI2, MISO2.
  // CS will be a GPIO output pin(configure and setup direction)
  // todo_configure_your_ssp2_pin_functions();

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
    const adesto_flash_id_s id = adesto_read_signature();

    // When we read a manufacturer ID we do not expect, we will kill this task
    if (id.manufacturer_id != 0x1F) {
      fprintf(stderr, "Manufacturer ID read failure\n");
      vTaskSuspend(NULL); // Kill this task
    }
  }
}
#endif
#endif

int main(void) {
#ifdef SPI_ASSIGNMENT
  const uint32_t spi_clock_mhz = 24;
  ssp2_lab_init(spi_clock_mhz);
  adesto_ds();
#ifdef PART1_SPI_ASSGNMT
  xTaskCreate(spi_task, "spi", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif
#ifdef PART2_SPI_ASSGNMT
  xTaskCreate(spi_id_verification_task, "id_verify", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(spi_id_verification_task, "id_verify1", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif
#else
  create_blinky_tasks();
  create_uart_task();
#endif
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
