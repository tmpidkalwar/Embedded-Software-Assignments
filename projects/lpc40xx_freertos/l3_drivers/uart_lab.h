/**
 * @file uart_lab.h
 * @author Tejas Pidkalwar
 * @date 13th oct 2020
 * @brief This file is responsible for UART driver initailization and maintainance of data receive and send
 * functionality
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  UART_2,
  UART_3,
} uart_number_e;

/**
 * Initialize the given UART peripheral
 * @param uart UART channel number to which data need to be written
 * @param peripheral_clock The peripheral clock frequency used for Baud rate configuration purpose
 * @param baud_rate The baud rate value need to be set for given UART config
 */
void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate);

/**
 * Read the byte from RBR and actually save it to the pointer
 * @param uart UART channel number to which data need to be written
 * @param *input_byte The address to which read data need to be written
 */
bool uart_lab__polled_get(uart_number_e uart, char *input_byte);

/**
 * Write the byte to the THR register based on status of space availibility
 * @param uart UART channel number to which data need to be written
 * @param output_byte The data to be written
 */
bool uart_lab__polled_put(uart_number_e uart, char output_byte);

/**
 * Public function to get a char from the queue (this function should work without modification)
 * @param *input_byte address to data to be read from queue
 * @param timeout maximum time limit to wait for the data in the queue
 */
bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout);

/**
 * Enables the data receive interrupt for the given UART channel
 * @param uart_numebr UART channel number to enable interrupt
 */
void uart__enable_receive_interrupt(uart_number_e uart_number);