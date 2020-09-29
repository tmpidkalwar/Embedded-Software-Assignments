/**
 * @file
 * Analog to Digital Converter driver for LPC40xx
 *
 * @note
 * This driver is intentionally simplified, and is meant to be used as a reference.
 * Burst mode should be enabled for a better implementation
 */
#pragma once

#include <stdint.h>

// Only Channel2, Channel4 and Channel5 pins are avaible for use on SJ2 Development board
typedef enum {
  ADC__CHANNEL_2 = 2, // Pin Available on SJ2
  ADC__CHANNEL_4 = 4, // Pin Available on SJ2
  ADC__CHANNEL_5 = 5, // Pin Available on SJ2
} adc_channel_e;

void adc__initialize(void);

/**
 * Reads the given ADC channal and returns its digital value
 * This starts conversion of one channel, and should not be used from multiple tasks
 */
uint16_t adc__get_adc_value(adc_channel_e channel_num);

/**
 * Implement a new function called adc__enable_burst_mode() which will
 * set the relevant bits in Control Register (CR) to enable burst mode.
 */
void adc__enable_burst_mode(void);

/**
 * Note:
 * The existing ADC driver is designed to work for non-burst mode
 *
 * You will need to write a routine that reads data while the ADC is in burst mode
 * Note that in burst mode, you will NOT read the result from the GDR register
 * Read the LPC user manual for more details
 */
uint16_t adc__get_channel_reading_with_burst_mode(adc_channel_e channel_number);
