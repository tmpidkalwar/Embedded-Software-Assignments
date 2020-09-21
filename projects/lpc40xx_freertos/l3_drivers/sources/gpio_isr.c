/**
 * @file gpio_isr.c
 * @author Tejas Pidkalwar
 * @date 20th Sept 2020
 * @brief This file maintains the abstracted APIs to configure GPIO interrupts
 */

#include "gpio_isr.h"
#include "stdio.h"

/**
 * Array of callback ISR functions for GPIO falling edge interrupts
 */
static function_pointer_t gpio0_falling_edge_callbacks[32];

/**
 * Array of callback ISR functions for GPIO rising edge interrupts
 */
static function_pointer_t gpio0_rising_edge_callbacks[32];

/**
 *  This function reads the status register to know which pin on GPIO0 has caused the interrupt
 *  @param interrupt_type The pointer to the type of interrupt
 *  @return pin return the pin that caused the interrupt
 */
static int who_generated_interrupt_on_gpio0(gpio_interrupt_e *interrupt_type) {
  // Update the interrupt type by reading status register
  *interrupt_type = (LPC_GPIOINT->IO0IntStatF) ? GPIO_INTR__FALLING_EDGE : GPIO_INTR__RISING_EDGE;

  // Read the corresponding interrupt status register
  volatile uint32_t status =
      (*interrupt_type == GPIO_INTR__FALLING_EDGE) ? (LPC_GPIOINT->IO0IntStatF) : (status = LPC_GPIOINT->IO0IntStatR);

  // get the pin number from the status register
  int pin = 0;
  while (status >>= 1)
    pin++;
  return pin;
}

/**
 *  Clear the interrupt on a given pin of port 0.
 *  @param pin pin number of which interrupt needs to be cleared
 */
static void gpio0_clear_pin_interrupt(uint32_t pin) { LPC_GPIOINT->IO0IntClr |= (1 << pin); }

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    gpio0_falling_edge_callbacks[pin] = callback;
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
  } else {
    gpio0_rising_edge_callbacks[pin] = callback;
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
  }
}

void gpio0__interrupt_dispatcher(void) {

  gpio_interrupt_e interrupt_type;

  // Check which pin generated the interrupt
  int pin_that_generated_interrupt = who_generated_interrupt_on_gpio0(&interrupt_type);

  function_pointer_t attached_user_handler;
  if (interrupt_type == GPIO_INTR__FALLING_EDGE)
    attached_user_handler = gpio0_falling_edge_callbacks[pin_that_generated_interrupt];
  else
    attached_user_handler = gpio0_rising_edge_callbacks[pin_that_generated_interrupt];

  // Invoke the user registered callback, and then clear the interrupt
  attached_user_handler();
  gpio0_clear_pin_interrupt(pin_that_generated_interrupt);
}