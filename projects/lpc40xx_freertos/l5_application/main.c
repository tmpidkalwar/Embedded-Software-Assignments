#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#ifdef ADC_PWM_ASSIGNMENT

#include "adc.h"
#include "lpc_peripherals.h"
#include "pwm1.h"
#include "semphr.h"

#endif

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

#ifdef ADC_PWM_ASSIGNMENT

QueueHandle_t adc_to_pwm_q;

void pwm_task() {
  pwm1__init_single_edge(1000);

  // Set port 2 pin 0 as PWM1 channel 0 as an PWM output
  gpio__construct_with_function(2, 0, GPIO__FUNCTION_1);

  // Initialize duty cycle to 50%
  pwm1__set_duty_cycle(PWM1__2_0, 50);

#ifdef PART0_ADC_PWM_ASSGNMT
  uint8_t percent = 0;
  bool count_down = false;
  while (1) {
    pwm1__set_duty_cycle(PWM1__2_0, percent);

    if (count_down) {
      percent -= 5;
      if (percent <= 0)
        count_down = false;
    } else {
      percent += 5;
      if (percent > 100)
        count_down = true;
    }
    vTaskDelay(50);
  }
#elif defined(PART2_ADC_PWM_ASSGNMT)
  uint8_t percent = 0;
  // Set port 2 pin 1 as PWM1 channel 1 as an PWM output
  gpio__construct_with_function(2, 1, GPIO__FUNCTION_1);

  // Set port 2 pin 2 as PWM1 channel 2 as an PWM output
  gpio__construct_with_function(2, 2, GPIO__FUNCTION_1);
  uint16_t max_adc_value = 4096;
  uint16_t convertedValue = 0;
  while (1) {
    if (xQueueReceive(adc_to_pwm_q, &convertedValue, 100)) {
      percent = (convertedValue * 100 / max_adc_value);
      pwm1__set_duty_cycle(PWM1__2_0, percent);
      pwm1__set_duty_cycle(PWM1__2_1, ((percent + 30) % 100));
      pwm1__set_duty_cycle(PWM1__2_2, ((percent + 60) % 100));
    }
  }
#endif
}

void adc_task() {
  adc__initialize();
  adc__enable_burst_mode();

  // Make Port 0 Pin 25 functionality as ADC0[2]
  gpio__construct_with_function(0, 25, GPIO__FUNCTION_1);

  // Set mode and analog input bits in control register
  LPC_IOCON->P0_25 &= ~(0b11 << 3);

  LPC_IOCON->P0_25 &= ~(1 << 7);
  uint16_t convertedValue = 0;
#ifdef PART1_ADC_PWM_ASSGNMT

  double current_voltage_value = 0;
  double max_adc_value = 4096;
  while (1) {
    // Read the ADC value in burst mode
    convertedValue = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    current_voltage_value = (convertedValue / max_adc_value) * (3.3);
    fprintf(stderr, "Current Pot Value is %d\n", convertedValue);
    fprintf(stderr, "Current voltage level is %f \n", current_voltage_value);
    vTaskDelay(1000);
  }
#elif defined(PART2_ADC_PWM_ASSGNMT)
  while (1) {
    // Read the ADC value in burst mode
    convertedValue = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    // Send the ADC value to PWM channels
    xQueueSend(adc_to_pwm_q, &convertedValue, 0);
    vTaskDelay(100);
  }
#endif
}

#endif

int main(void) {

#ifdef ADC_PWM_ASSIGNMENT

#if defined(PART0_ADC_PWM_ASSGNMT) || defined(PART2_ADC_PWM_ASSGNMT)

  xTaskCreate(pwm_task, "pwm", 1024 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

#endif

#if defined(PART1_ADC_PWM_ASSGNMT) || defined(PART2_ADC_PWM_ASSGNMT)

  xTaskCreate(adc_task, "adc", 1024 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

#endif

#ifdef PART2_ADC_PWM_ASSGNMT

  adc_to_pwm_q = xQueueCreate(10, sizeof(uint16_t));

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
