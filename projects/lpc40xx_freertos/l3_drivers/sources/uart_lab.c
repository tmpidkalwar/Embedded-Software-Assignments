/**
 * @file uart_lab.c
 * @author Tejas Pidkalwar
 * @date 13th oct 2020
 * @brief This file is responsible for UART driver initailization and maintainance of data receive and send
 * functionality
 */

#include "uart_lab.h"
#include "assert.h"
#include "clock.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "stdio.h"

#if defined(PART2_UART_ASSGNMT) || defined(PART3_UART_ASSGNMT)
#include "FreeRTOS.h"
#include "queue.h"

static QueueHandle_t uart_rx_queue;
#endif

typedef LPC_UART_TypeDef lpc_reg;

/*******************************************************************************
 *
 *                      P R I V A T E    F U N C T I O N S
 *
 ******************************************************************************/

static lpc_reg *uarts[] = {(lpc_reg *)LPC_UART2, (lpc_reg *)LPC_UART3};
const uint8_t dlab_bit_mask = (1 << 7);
const uint8_t lsr_reg_received_data_ready_bit_mask = (1 << 0);

static void set_dlab_bit(uart_number_e uart) { uarts[uart]->LCR |= dlab_bit_mask; }
static void clear_dlab_bit(uart_number_e uart) { uarts[uart]->LCR &= ~dlab_bit_mask; }

#if defined(PART2_UART_ASSGNMT) || defined(PART3_UART_ASSGNMT)
/**
 * UART2 receiver interrupt handler fucntion
 */
static void uart2_receive_interrupt(void) {
  // Read the IIR register to figure out why you got interrupted
  const uint8_t interrupt_id_mask = (0b111);
  const uint8_t interrupt_pending_bit = (1 << 0);
  uint8_t interrupt_id;
  if (!(LPC_UART2->IIR & interrupt_pending_bit))
    interrupt_id = ((LPC_UART2->IIR >> 1) & interrupt_id_mask);
  else {
    fprintf(stderr, "UART Interrupt occured due to unconfigured UART cannel, habe look into it");
    assert(0);
  }

  if ((interrupt_id == 2) && (LPC_UART2->LSR & lsr_reg_received_data_ready_bit_mask)) {
    const char byte = LPC_UART2->RBR;
    xQueueSendFromISR(uart_rx_queue, &byte, NULL);
    // fprintf(stderr, "I am in ISR, Just put %X data in queue\n", byte);
  }
}

/**
 * UART3 receiver interrupt handler fucntion
 */
static void uart3_receive_interrupt(void) {
  const uint8_t interrupt_id_mask = (0b111);
  const uint8_t interrupt_pending_bit = (1 << 0);
  uint8_t interrupt_id;
  // Check whether there is pending interrupt corresponding to UART3
  if (!(LPC_UART3->IIR & interrupt_pending_bit))
    interrupt_id = ((LPC_UART3->IIR >> 1) & interrupt_id_mask);
  else {
    fprintf(stderr, "UART Interrupt occured due to unconfigured UART cannel, habe look into it");
    assert(0);
  }

  if ((interrupt_id == 2) && (LPC_UART3->LSR & lsr_reg_received_data_ready_bit_mask)) {
    const char byte = LPC_UART3->RBR;
    xQueueSendFromISR(uart_rx_queue, &byte, NULL);
    // printf("I am in ISR, Just put %X data in queue\n", byte);
  }
}
#endif

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Power on the peripheral
  uint32_t uart_pconp_mask;
  if (uart == UART_2)
    uart_pconp_mask = (0x1 << 24);
  else
    uart_pconp_mask = (0x1 << 25);
  LPC_SC->PCONP |= uart_pconp_mask;

  lpc_reg *uart_reg = uarts[uart];

  // Setup the Baud rate
  // UART_baud_rate = (PCLK / (16 * ((DLM << 8) + DLL)))
  set_dlab_bit(uart);
  uint16_t DLs = peripheral_clock / (16 * baud_rate);
  uart_reg->DLL |= (DLs & 0xFF);
  uart_reg->DLM |= (DLs & (0xFF << 8));
  clear_dlab_bit(uart);

  // Configure LCR register
  const uint8_t word_length_bit_mask = (3 << 0);
  const uint8_t word_length_8bits_value_mask = (3 << 0);
  uart_reg->LCR &= ~(word_length_bit_mask);
  uart_reg->LCR |= (word_length_8bits_value_mask);

  // Select UART pins and modes using IOCON
  if (uart == UART_2) {
    gpio__construct_with_function(GPIO__PORT_2, 8, GPIO__FUNCTION_2);
    gpio__construct_with_function(GPIO__PORT_2, 9, GPIO__FUNCTION_2);

  } else {
    gpio__construct_with_function(GPIO__PORT_4, 28, GPIO__FUNCTION_2);
    gpio__construct_with_function(GPIO__PORT_4, 29, GPIO__FUNCTION_2);
  }
}

bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  clear_dlab_bit(uart);

  if (uarts[uart]->LSR & lsr_reg_received_data_ready_bit_mask) {
    *input_byte = uarts[uart]->RBR;
    return true;
  }
  return false;
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  clear_dlab_bit(uart);
  const uint8_t lsr_reg_transmit_buffer_empty_bit_mask = (1 << 5);
  if (uarts[uart]->LSR & lsr_reg_transmit_buffer_empty_bit_mask) {
    uarts[uart]->THR |= output_byte;
    while (!(uarts[uart]->LSR & lsr_reg_transmit_buffer_empty_bit_mask)) {
    }
    return true;
  }
  return false;
}

#if defined(PART2_UART_ASSGNMT) || defined(PART3_UART_ASSGNMT)
void uart__enable_receive_interrupt(uart_number_e uart_number) {
  const uint8_t receive_data_enable_interrupt_bit_mask = (1 << 0);
  // Enable UART receive interrupt by setting IER register bit
  if (uart_number == UART_2) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, uart2_receive_interrupt, "UART2 data received");
    LPC_UART2->IER |= receive_data_enable_interrupt_bit_mask;
  } else {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, uart3_receive_interrupt, "UART2 data received");
    LPC_UART3->IER |= receive_data_enable_interrupt_bit_mask;
  }

  // Create your RX queue
  uart_rx_queue = xQueueCreate(10, sizeof(char));
}

bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(uart_rx_queue, input_byte, timeout);
}

#endif