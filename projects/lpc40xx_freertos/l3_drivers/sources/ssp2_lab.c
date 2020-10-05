/**
 * @file
 * @author
 * @date
 * @brief
 */

#include "clock.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdint.h>
#include <stdio.h>

void ssp2_lab_init(uint32_t max_clock_mhz) {
  // Power on SSP2 peripheral
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP2);

  const uint32_t cpu_clock_mhz = clock__get_core_clock_hz() / (1000 * 1000);
  // Clock diviser for SSP2 as 4
  LPC_SSP2->CPSR &= ~(0xFF);
  LPC_SSP2->CPSR |= (cpu_clock_mhz / max_clock_mhz);

  // Select SSP pin clock
  gpio__construct_with_function(GPIO__PORT_1, 0, GPIO__FUNCTION_4);

  // Select SSP pin MOSI
  gpio__construct_with_function(GPIO__PORT_1, 1, GPIO__FUNCTION_4);

  // Select SSP pin MISO
  gpio__construct_with_function(GPIO__PORT_1, 4, GPIO__FUNCTION_4);

  // Select CS
  gpio__construct_as_output(GPIO__PORT_1, 10);

  // Configure CR0 register
  LPC_SSP2->CR0 |= (0b111 << 0);

  // Configure CR1 register
  LPC_SSP2->CR1 |= (1 << 1);
}

uint8_t ssp2_lab_exchange_byte(uint8_t data_out) {
  // Update Data Register with the given Data
  LPC_SSP2->DR = data_out;
  while ((LPC_SSP2->SR & (1 << 4))) {
    ;
  }
  return LPC_SSP2->DR;
}
