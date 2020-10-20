#include <stdio.h>

#include "FreeRTOS.h"
//#include "assignment_include.h"
#include "semphr.h"
#include "task.h"

#include "cpu_utilization_task.h"

typedef enum { switch__off, switch__on } switch_e;

//#ifdef PROD_CONS_ASSIGNMENT
static QueueHandle_t prod_to_cons_queue;

static switch_e get_switch_input_from_switch(void) { return switch__on; }

void producer_task(void *p) {
  while (1) {
    const switch_e switch_value = get_switch_input_from_switch();
    printf("Ping from Before sending the msg\n");
    if (!xQueueSend(prod_to_cons_queue, &switch_value, 0)) {
      fprintf(stderr, "Unable to send data over RTOS queue");
    }
    printf("Ping after sending %d data over the RTOS queue\n", switch_value);
    vTaskDelay(500);
  }
}

void consumer_task(void *p) {
  switch_e switch_value = switch__off;
  while (1) {
    if (xQueueReceive(prod_to_cons_queue, &switch_value, 0)) {
      printf("The data %d is received over the RTOS queue\n", switch_value);
    }
  }
}

//#endif
/**
 * This POSIX based FreeRTOS simulator is based on:
 * https://github.com/linvis/FreeRTOS-Sim
 *
 * Do not use for production!
 * There may be issues that need full validation of this project to make it production intent.
 * This is a great teaching tool though :)
 */
int main(void) {
  //#ifdef PROD_CONS_ASSIGNMENT
  prod_to_cons_queue = xQueueCreate(10, sizeof(switch_e));
  xTaskCreate(producer_task, "Producer", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(consumer_task, "Consumer", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  //#else
  // xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);
  //#endif
  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();

  return 0;
}
