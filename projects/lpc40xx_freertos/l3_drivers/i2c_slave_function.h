#pragma once
#include <stdbool.h>
#include <stdint.h>

/**
 * This function is responsible for reading slave memory
 * @param memory_index The index of Slave memory need to be read
 * @param *memory Reference of variable to be updated with salve memory data
 */
bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory);

/**
 * This function is responsible for writing slave memory
 * @param memory_index The index of Slave memory need to be read
 * @param *memory Reference of variable to be updated with salve memory data
 */
bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value);