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

// include this file at gpio_lab.c file
// #include "lpc40xx.h"

// NOTE: The IOCON is not part of this driver

// Alter the hardware registers to set the pin as input
void gpio0__set_as_input(uint8_t pin_num);

// Alter the hardware registers to set the pin as output
void gpio0__set_as_output(uint8_t pin_num);

// Alter the hardware registers to set the pin as high
void gpio0__set_high(uint8_t pin_num);

// Alter the hardware registers to set the pin as low
void gpio0__set_low(uint8_t pin_num);

/**
 * Alter the hardware registers to set the pin as low
 * @param {uint8_t} pin_num => input the pin number want to access
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpio0__set(uint8_t pin_num, bool high);

/**
 * Should return the state of the pin (input or output, doesn't matter)
 * @param {uint8_t} pin_num => input the pin number want to access
 * @return {bool} level of pin high => true, low => false
 */
bool gpio0__get_level(uint8_t pin_num);

// Alter the hardware registers to set the pin as input
void gpio1__set_as_input(uint8_t pin_num);

// Alter the hardware registers to set the pin as output
void gpio1__set_as_output(uint8_t pin_num);

// Alter the hardware registers to set the pin as high
void gpio1__set_high(uint8_t pin_num);

// Alter the hardware registers to set the pin as low
void gpio1__set_low(uint8_t pin_num);

/**
 * Alter the hardware registers to set the pin as low
 * @param {uint8_t} pin_num => input the pin number want to access
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpio1__set(uint8_t pin_num, bool high);

/**
 * Should return the state of the pin (input or output, doesn't matter)
 * @param {uint8_t} pin_num => input the pin number want to access
 * @return {bool} level of pin high => true, low => false
 */
bool gpio1__get_level(uint8_t pin_num);