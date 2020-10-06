/**
 * @file adesto_flash.c
 * @author Tejas Pidkalwar
 * @date 19th Sept 2020
 * @brief This file maintains the macro defined to enable specific assignment implementation for
 *      execution, along with subparts of assignment enabling feature
 */
#ifndef _ADESTO_FLASH_
#define _ADESTO_FLASH_

#include "gpio.h"
#include "lpc40xx.h"

/**
 * the Adesto flash 'Manufacturer and Device ID' section
 */
typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_1;
  uint8_t device_id_2;
  uint8_t extended_device_id;
} adesto_flash_id_s;

/**
 * Assert the Slave select pin for SSP2 driver
 */
void adesto_cs(void);

/**
 * De-assert the Slave select pin for SSP2 driver
 */
void adesto_ds(void);

/**
 * Read the Flash manufacturing and device id information
 * @return flash id structure with read information
 */
adesto_flash_id_s adesto_read_signature(void);

/**
 * Reads the page of a flash at given address
 */
void adesto_flash_page_read(void);

/**
 * Writes the page of a flash at given address
 */
void adesto_flash_page_write(void);

#endif