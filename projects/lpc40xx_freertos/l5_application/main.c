#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#ifdef WATCHDOG_ASSGNMT

#include "acceleration.h"
#include "event_groups.h"
#include "ff.h"
#include "portmacro.h"
#include "semphr.h"
#include <string.h>

// #define PRODUCER_TASK_CPU_UTIL_CHECK
// #define FILE_TRANSFER_CPU_UTIL_CHECK

#define CONS_EVENT_BIT (1 << 1)
#define PROD_EVENT_BIT (1 << 2)

static void producer_task(void *p);
static void consumer_task(void *p);
static void watchdog_task(void *p);
static void create_uart_task(void);
static void uart_task(void *params);

QueueHandle_t queue_to_cons_task;
EventGroupHandle_t sensor_events;

#else
// 'static' to make these functions 'private' to this file
static void create_blinky_tasks(void);
static void blink_task(void *params);
#endif

int main(void) {
#ifdef WATCHDOG_ASSGNMT

  queue_to_cons_task = xQueueCreate(10, sizeof(acceleration__axis_data_s));
  sensor_events = xEventGroupCreate();
  xTaskCreate(producer_task, "producer", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(consumer_task, "consumer", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(watchdog_task, "watchdog", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  create_uart_task();
#else
  create_blinky_tasks();

  // If you have the ESP32 wifi module soldered on the board, you can try uncommenting this code
  // See esp32/README.md for more details
  // uart3_init();                                                                     // Also include:  uart3_init.h
  // xTaskCreate(esp32_tcp_hello_world_task, "uart3", 1000, NULL, PRIORITY_LOW, NULL); // Include esp32_task.h
#endif
  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

#ifdef WATCHDOG_ASSGNMT

static void write_sensor_data_to_file(acceleration__axis_data_s sensor_data) {
  const char *filename = "sensor.txt";
  FIL file; // File handle
  UINT bytes_written = 0;
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_OPEN_APPEND));

  TickType_t time = xTaskGetTickCount();
  if (FR_OK == result) {
    char string[64];
    sprintf(string, "%li, %i, %i, %i\n", time, sensor_data.x, sensor_data.y, sensor_data.z);
    if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
    } else {
      printf("ERROR: Failed to write data to file\n");
    }
    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }
}

static void write_watchdog_logs_to_SD(char *string) {
  const char *filename = "watchdog_logs.txt";
  FIL file; // File handle
  UINT bytes_written = 0;
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_OPEN_APPEND));
  TickType_t time = xTaskGetTickCount();

  sprintf(string + strlen(string), "\t %li, \n", time);

  if (FR_OK == result) {
    if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
    } else {
      printf("ERROR: Failed to write data to file\n");
    }
    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }
}

void producer_task(void *p) {
  static int count;
  acceleration__axis_data_s read_acceler_data;
  acceleration__axis_data_s avg_acceler_value = {0};

  while (1) {
    TickType_t time = xTaskGetTickCount();
    read_acceler_data = acceleration__get_data();

    // Update average sensed values
    avg_acceler_value.x = (avg_acceler_value.x + read_acceler_data.x) / 2;
    avg_acceler_value.y = (avg_acceler_value.y + read_acceler_data.y) / 2;
    avg_acceler_value.z = (avg_acceler_value.z + read_acceler_data.z) / 2;

    // If we are done reading 100 values, send average value to consumer task
    if (++count >= 100) {
      if (!xQueueSend(queue_to_cons_task, &avg_acceler_value, 300))
        fprintf(stderr, "failed to send data to consumer task\n");
      else {
        count = 0;
        memset(&avg_acceler_value, 0, sizeof(avg_acceler_value));
      }
    }
    xEventGroupSetBits(sensor_events, PROD_EVENT_BIT);
    time = xTaskGetTickCount() - time;
#ifdef PRODUCER_TASK_CPU_UTIL_CHECK
    fprintf(stderr, "CPU Utilization by Producer Task is %ld RTOS ticks\n", time);
#endif
    vTaskDelay(2);
  }
}

void consumer_task(void *p) {
  acceleration__axis_data_s received_data[20];
  static int count;
  while (1) {
    xEventGroupSetBits(sensor_events, CONS_EVENT_BIT);
    if (xQueueReceive(queue_to_cons_task, &received_data[count], 100)) {
      if (++count >= 20) {
        for (int i = 0; i < count; i++) {
          write_sensor_data_to_file(received_data[i]);
        }
      }
    }
  }
}

void watchdog_task(void *p) {
  while (1) {
    EventBits_t uxBits;
    uxBits = xEventGroupWaitBits(sensor_events, (PROD_EVENT_BIT | CONS_EVENT_BIT), pdFALSE, pdTRUE, 2100);

    char string[128];
    if ((uxBits & (PROD_EVENT_BIT | CONS_EVENT_BIT)) == (PROD_EVENT_BIT | CONS_EVENT_BIT)) {
      printf("PRODUCER and CONSUMER TASKS checked in successfully\n");
      sprintf(string, "PRODUCER and CONSUMER TASKS checked in successfully\n");
    } else if ((uxBits & PROD_EVENT_BIT) == PROD_EVENT_BIT) {
      printf("PRODUCER TASK checked in successfully\n");
      printf("ERROR: CONSUMER TASK failed to check in\n");
      sprintf(string, "PRODUCER TASK checked in successfully\n");
      sprintf(string + strlen(string), "ERROR: CONSUMER TASK failed to check in\n");
    } else if ((uxBits & CONS_EVENT_BIT) == CONS_EVENT_BIT) {
      printf("CONSUMER TASK checked in successfully\n");
      printf("ERROR: PRODUCER TASK failed to check in\n");
      sprintf(string, "CONSUMER TASK checked in successfully\n");
      sprintf(string + strlen(string), "ERROR: PRODUCER TASK failed to check in\n");
    } else {
      printf("ERROR: PRODUCER TASK and CONSUMER TASK failed to check in\n");
      sprintf(string, "ERROR: PRODUCER TASK and CONSUMER TASK failed to check in\n");
    }
    TickType_t time = xTaskGetTickCount();
    write_watchdog_logs_to_SD(string);
    time = xTaskGetTickCount() - time;
#ifdef FILE_TRANSFER_CPU_UTIL_CHECK
    printf("CPU utilization during file write is %ld ticks\n", time);
#endif
    xEventGroupClearBits(sensor_events, CONS_EVENT_BIT);
    xEventGroupClearBits(sensor_events, PROD_EVENT_BIT);
  }
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

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

#endif