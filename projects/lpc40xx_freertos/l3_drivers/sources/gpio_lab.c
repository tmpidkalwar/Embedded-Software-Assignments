/**
 * @file gpio_lab.c
 * @author Tejas Pidkalwar
 * @date 11th Sept 2020
 * @brief This file contains the implementations of register bit manipulating
 *      functions for GPIO0 and GPIO1 as a part of LED_TASK assignment.
 */

#include "gpio_lab.h"

#include "lpc40xx.h"

/**
 * array of pointer ro GPIO ports' base addresses
 */
static LPC_GPIO_TypeDef *gpio_lab_port_memory_map[] = {LPC_GPIO0, LPC_GPIO1, LPC_GPIO2,
                                                       LPC_GPIO3, LPC_GPIO4, LPC_GPIO5};

/**
 * Retruns the pointer to an base address of an given port
 * @param gpio GPIO structure of which base address we need
 * @return (LPC_GPIO_TypeDef *) pointer to the base address of given port
 */
static LPC_GPIO_TypeDef *gpio_lab_get_struct(gpio_lab_s gpio) {
  return (LPC_GPIO_TypeDef *)gpio_lab_port_memory_map[gpio.port_number];
}

/**
 * Retruns the pin masked value for the given GPIO
 * @param gpio GPIO structure of which base address we need
 * @return pin masked value for the given GPIO
 */
static uint32_t gpio_lab_get_pin_mask(gpio_lab_s gpio) { return (1 << gpio.pin_number); }

void gpio_lab_set_as_input(gpio_lab_s gpio) { gpio_lab_get_struct(gpio)->DIR &= ~gpio_lab_get_pin_mask(gpio); }

void gpio_lab_set_as_output(gpio_lab_s gpio) { gpio_lab_get_struct(gpio)->DIR |= gpio_lab_get_pin_mask(gpio); }

void gpio_lab_set_high(gpio_lab_s gpio) { gpio_lab_get_struct(gpio)->SET = gpio_lab_get_pin_mask(gpio); }

void gpio_lab_set_low(gpio_lab_s gpio) { gpio_lab_get_struct(gpio)->CLR = gpio_lab_get_pin_mask(gpio); }

void gpio_lab_set(gpio_lab_s gpio, bool high) {
  // based on the high bit's input value manipulate GPIO PIN.
  if (high)
    gpio_lab_get_struct(gpio)->SET = gpio_lab_get_pin_mask(gpio);
  else
    gpio_lab_get_struct(gpio)->CLR = gpio_lab_get_pin_mask(gpio);
}

bool gpio_lab_get_level(gpio_lab_s gpio) { return (gpio_lab_get_struct(gpio)->PIN & gpio_lab_get_pin_mask(gpio)); }