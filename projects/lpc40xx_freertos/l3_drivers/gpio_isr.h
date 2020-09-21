/**
 * @file gpio_isr.h
 * @author Tejas Pidkalwar
 * @date 20th Sept 2020
 * @brief This file maintains the abstracted APIs to configure GPIO interrupts
 */

#pragma once

#include "lpc40xx.h"
#include <stdint.h>

typedef enum {
  GPIO_INTR__FALLING_EDGE,
  GPIO_INTR__RISING_EDGE,
} gpio_interrupt_e;

// Function pointer type (demonstrated later in the code sample)
typedef void (*function_pointer_t)(void);

/**
 * Allow the user to attach their callbacks
 * @param pin GPIO pin to be configured for GPIO interrupt
 * @param interrupt_type Type of interrupt configuration
 * @param callback An ISR function to be called on occurance of this interrupt
 */
void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);

// Our main() should configure interrupts to invoke this dispatcher where we will invoke user attached callbacks
// You can hijack 'interrupt_vector_table.c' or use API at lpc_peripherals.h
void gpio0__interrupt_dispatcher(void);