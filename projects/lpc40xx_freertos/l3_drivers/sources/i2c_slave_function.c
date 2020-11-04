#include "i2c_slave_function.h"
#include "i2c.h"

#define SLAVE_MEMORY_SIZE 10

/**
 * Register number = memory Index
 * Reg 0: On/Off
 * |  Led 3  |  Led 2  | Led 1  |  Led 0  |       |       |        |      |
 *                                        |         Reserved              |
 *
 * Reg 1: Blinking
 * | Led3  |  Led2 | Led 1| Led 0|     |      |      |  on/off(blinking)|
 *                               |  Blinking duration|
 */

static volatile uint8_t slave_memory[SLAVE_MEMORY_SIZE];

bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  if (memory_index < SLAVE_MEMORY_SIZE) {
    *memory = slave_memory[memory_index];
  } else {
    return false;
  }
  return true;
}

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  if (memory_index < SLAVE_MEMORY_SIZE)
    slave_memory[memory_index] = memory_value;
  else
    return false;
  return true;
}