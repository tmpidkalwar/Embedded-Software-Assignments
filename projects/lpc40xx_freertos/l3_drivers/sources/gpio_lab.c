/**
 * @file gpio_lab.c
 * @author Tejas Pidkalwar
 * @date 11th Sept 2020
 * @brief This file contains the implementations of register bit manipulating
 *      functions for GPIO0 and GPIO1 as a part of LED_TASK assignment.
 */

#include "gpio_lab.h"

#include "lpc40xx.h"

void gpio0__set_as_input(uint8_t pin_num) { LPC_GPIO0->DIR &= ~(1 << pin_num); }

void gpio0__set_as_output(uint8_t pin_num) { LPC_GPIO0->DIR |= (1 << pin_num); }

void gpio0__set_high(uint8_t pin_num) { LPC_GPIO0->SET = (1 << pin_num); }

void gpio0__set_low(uint8_t pin_num) { LPC_GPIO0->CLR = (1 << pin_num); }

void gpio0__set(uint8_t pin_num, bool high) {
  // based on the high bit's input value manipulate GPIO PIN.
  if (high)
    LPC_GPIO0->SET = (1 << pin_num);
  else
    LPC_GPIO0->CLR = (1 << pin_num);
}

bool gpio0__get_level(uint8_t pin_num) { return (LPC_GPIO0->PIN & (1 << pin_num)); }