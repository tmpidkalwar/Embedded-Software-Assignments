/**
 * @file gpio_lab.h
 * @author Tejas Pidkalwar
 * @date 11th Sept 2020
 * @brief This file contains the prototypes of register bit manipulating
 *      functions for GPIO0 and GPIO1 as a part of LED_TASK assignment.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * This struct is returned gpio__construct()
 * All further APIs need instance of this struct to operate
 */
typedef struct {
  uint8_t port_number : 3;
  uint8_t pin_number : 5;
} gpio_lab_s;

// NOTE: The IOCON is not part of this driver

/**
 * Alter the hardware registers to set the pin as input
 * @param gpio GPIO structure of which base address we need
 */
void gpio_lab_set_as_input(gpio_lab_s gpio);

/**
 * Alter the hardware registers to set the pin as output
 * @param gpio GPIO structure of which base address we need
 */
void gpio_lab_set_as_output(gpio_lab_s gpio);

/**
 * Alter the hardware registers to set the pin as high
 * @param gpio GPIO structure of which base address we need
 */
void gpio_lab_set_high(gpio_lab_s gpio);

/**
 * Alter the hardware registers to set the pin as low
 * @param gpio GPIO structure of which base address we need
 */
void gpio_lab_set_low(gpio_lab_s gpio);

/**
 * Alter the hardware registers to set the pin as low
 * @param {uint8_t} pin_num => input the pin number want to access
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpio_lab_set(gpio_lab_s gpio, bool high);

/**
 * Should return the state of the pin (input or output, doesn't matter)
 * @param {uint8_t} pin_num => input the pin number want to access
 * @return {bool} level of pin high => true, low => false
 */
bool gpio_lab_get_level(gpio_lab_s gpio);