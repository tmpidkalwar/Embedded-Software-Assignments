#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "cpu_utilization_task.h"

QueueHandle_t signal_transfer_queue;

void signal_generator_task(void *p) {
  uint16_t signal = 0xDADA;
  while (1) {
    xQueueSend(signal_transfer_queue, &signal, 0);
    vTaskDelay(1000);
  }
}

void signal_consumer_task(void *p) {
  uint16_t signal;
  while (1) {
    xQueueReceive(signal_transfer_queue, &signal, portMAX_DELAY);
    printf("Received signal is %X\n", signal);
  }
}

/**
 * This POSIX based FreeRTOS simulator is based on:
 * https://github.com/linvis/FreeRTOS-Sim
 *
 * Do not use for production!
 * There may be issues that need full validation of this project to make it production intent.
 * This is a great teaching tool though :)
 */
int main(void) {
  signal_transfer_queue = xQueueCreate(10, sizeof(uint16_t));
  xTaskCreate(signal_generator_task, "producer", 1024 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(signal_consumer_task, "consumer", 1024 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  // xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);

  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();

  return 0;
}
