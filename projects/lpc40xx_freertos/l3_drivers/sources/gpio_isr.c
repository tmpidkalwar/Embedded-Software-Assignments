/**
 * @file gpio_isr.c
 * @author Tejas Pidkalwar
 * @date 20th Sept 2020
 * @brief This file maintains the abstracted APIs to configure GPIO interrupts
 */

#include "gpio_isr.h"
#include "gpio.h"
#include "stdio.h"
/***************************GPIO 0********************************************/

/**
 * Array of callback ISR functions for GPIO 0 falling edge interrupts
 */
static function_pointer_t gpio0_falling_edge_callbacks[32];

/**
 * Array of callback ISR functions for GPIO 0 rising edge interrupts
 */
static function_pointer_t gpio0_rising_edge_callbacks[32];

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    gpio0_falling_edge_callbacks[pin] = callback;
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
  } else {
    gpio0_rising_edge_callbacks[pin] = callback;
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
  }
}

/***************************GPIO 2********************************************/

/**
 * Array of callback ISR functions for GPIO 2 falling edge interrupts
 */
static function_pointer_t gpio2_falling_edge_callbacks[32];

/**
 * Array of callback ISR functions for GPIO 2 rising edge interrupts
 */
static function_pointer_t gpio2_rising_edge_callbacks[32];

void gpio2__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    gpio2_falling_edge_callbacks[pin] = callback;
    LPC_GPIOINT->IO2IntEnF |= (1 << pin);
  } else {
    gpio2_rising_edge_callbacks[pin] = callback;
    LPC_GPIOINT->IO2IntEnR |= (1 << pin);
  }
}

/********************Common APIs**********************************************/

/**
 *  This function reads the status register to know which pin on GPIO0 has caused the interrupt
 *  @param interrupt_type The pointer to the type of interrupt
 *  @return pin return the pin that caused the interrupt
 */
static void update_interrupt_gpio_struct(gpio_interrupt_e *interrupt_type, gpio_s *gpio) {
  uint32_t status;

  // Read the GPIO interrupt registers to know interrupt causing GPIO specs
  if (LPC_GPIOINT->IO0IntStatF) {
    status = LPC_GPIOINT->IO0IntStatF;
    *interrupt_type = GPIO_INTR__FALLING_EDGE;
    gpio->port_number = 0;
  } else if (LPC_GPIOINT->IO0IntStatR) {
    status = LPC_GPIOINT->IO0IntStatR;
    *interrupt_type = GPIO_INTR__RISING_EDGE;
    gpio->port_number = 0;
  } else if (LPC_GPIOINT->IO2IntStatF) {
    status = LPC_GPIOINT->IO2IntStatF;
    *interrupt_type = GPIO_INTR__FALLING_EDGE;
    gpio->port_number = 1;
  } else {
    status = LPC_GPIOINT->IO2IntStatR;
    *interrupt_type = GPIO_INTR__RISING_EDGE;
    gpio->port_number = 1;
  }

  // get the pin number from the status register
  int pin = 0;
  while (status >>= 1)
    pin++;
  gpio->pin_number = pin;
}

/**
 *  Clear the interrupt on a given pin of port 0.
 *  @param pin pin number of which interrupt needs to be cleared
 */
static void gpio_clear_pin_interrupt(gpio_s gpio) {
  if (gpio.port_number == 0)
    LPC_GPIOINT->IO0IntClr |= (1 << gpio.pin_number);
  else
    LPC_GPIOINT->IO2IntClr |= (1 << gpio.pin_number);
}

void gpio__interrupt_dispatcher(void) {

  gpio_interrupt_e interrupt_type;
  gpio_s gpio;

  // Check which pin generated the interrupt
  update_interrupt_gpio_struct(&interrupt_type, &gpio);

  function_pointer_t attached_user_handler;
  if (gpio.port_number == 0) {
    if (interrupt_type == GPIO_INTR__FALLING_EDGE)
      attached_user_handler = gpio0_falling_edge_callbacks[gpio.pin_number];
    else
      attached_user_handler = gpio0_rising_edge_callbacks[gpio.pin_number];
  } else {
    if (interrupt_type == GPIO_INTR__FALLING_EDGE)
      attached_user_handler = gpio0_falling_edge_callbacks[gpio.pin_number];
    else
      attached_user_handler = gpio0_rising_edge_callbacks[gpio.pin_number];
  }

  // Invoke the user registered callback, and then clear the interrupt
  attached_user_handler();
  gpio_clear_pin_interrupt(gpio);
}