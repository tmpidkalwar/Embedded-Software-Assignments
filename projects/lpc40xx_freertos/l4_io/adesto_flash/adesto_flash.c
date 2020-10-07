/**
 * @file adesto_flash.c
 * @author Tejas Pidkalwar
 * @date 1st Oct 2020
 * @brief This file contains the Adesto flash configuration APIs
 */

#include "adesto_flash.h"
#include "ssp2_lab.h"
#include "stdio.h"

const uint8_t dummy_byte = 0xFF;

// TODO: Implement Adesto flash memory CS signal as a GPIO driver
void adesto_cs(void) {
  LPC_GPIO1->CLR = (1 << 10);
  LPC_GPIO0->CLR = (1 << 6);
}
void adesto_ds(void) {
  LPC_GPIO1->SET = (1 << 10);
  LPC_GPIO0->SET = (1 << 6);
}

#if defined(PART1_SPI_ASSGNMT) || defined(PART2_SPI_ASSGNMT)

// TODO: Implement the code to read Adesto flash memory signature
// TODO: Create struct of type 'adesto_flash_id_s' and return it
adesto_flash_id_s adesto_read_signature(void) {
  adesto_flash_id_s data = {0};
  const uint8_t flash_id_read_opcode = 0x9F;
  adesto_cs();
  {
    // Send the opcode to trigger manufacturing ID read from flash
    ssp2_lab_exchange_byte(flash_id_read_opcode);
    // read out the manufacturing id from Flash by sending dummy bytes
    data.manufacturer_id = ssp2_lab_exchange_byte(dummy_byte);
    data.device_id_1 = ssp2_lab_exchange_byte(dummy_byte);
    data.device_id_2 = ssp2_lab_exchange_byte(dummy_byte);
    data.extended_device_id = ssp2_lab_exchange_byte(dummy_byte);
  }
  adesto_ds();

  return data;
}

#endif

#ifdef PART3_SPI_ASSGNMT

/**
 * Send the 3 Bytes of address from the given one
 * @param address Address bytes to be sent
 */
static void adesto_flash_send_address(uint32_t address) {
  (void)ssp2_lab_exchange_byte((address >> 16) & 0xFF);
  (void)ssp2_lab_exchange_byte((address >> 8) & 0xFF);
  (void)ssp2_lab_exchange_byte((address >> 0) & 0xFF);
}

/**
 * Set the write enable latch WEL bit of status register byte 1
 */
static void adesto_flash_WEL_bit_set(void) {
  const uint8_t wel_bit_set_opcode = 0x06;
  adesto_cs();
  ssp2_lab_exchange_byte(wel_bit_set_opcode);
  adesto_ds();
}

/**
 * Erase the 4K block of memory starting from given address
 * @param erase_address Block's starting address to be erased
 */
static void adesto_flash_erase_4k_block(const uint32_t erase_address) {
  uint8_t status_reg_read_opcode = 0x05;
  uint8_t status_byte1 = 0x5;
  uint8_t erase_4k_block_opcode = 0x20;
  // Start of Erase Sequence
  adesto_flash_WEL_bit_set();
  adesto_cs();
  ssp2_lab_exchange_byte(erase_4k_block_opcode);
  adesto_flash_send_address(erase_address);
  adesto_ds();

  // Wait for erase operation to complete by polling on flash status register's Busy bit
  adesto_cs();
  ssp2_lab_exchange_byte(status_reg_read_opcode);
  do {
    status_byte1 = ssp2_lab_exchange_byte(dummy_byte);
  } while (status_byte1 & (1 << 0));
  adesto_ds();
  fprintf(stderr, "Current status register value byte1 is %x and hence ", status_byte1);
  fprintf(stderr, "Erased successfully\n    =>");
}

void adesto_flash_page_write(void) {
  const uint8_t page_write_opcode = 0x02;
  const uint32_t address_to_write = 0x000000;
  const uint8_t data = 0xDA;
  // Erase the memory to be written
  adesto_flash_erase_4k_block(address_to_write);

  // Set WEL bit of Status register
  adesto_flash_WEL_bit_set();

  // Start the flash write sequence
  adesto_cs();
  ssp2_lab_exchange_byte(page_write_opcode);
  adesto_flash_send_address(address_to_write);
  ssp2_lab_exchange_byte(data);
  adesto_ds();
  fprintf(stderr, " writen data '%X' successfully\n         =>", data);
}

void adesto_flash_page_read(void) {
  uint8_t read_data = 0x00;
  const uint32_t address_to_write = 0x000000;
  const uint8_t page_read_opcode = 0x0B;
  adesto_cs();
  ssp2_lab_exchange_byte(page_read_opcode);
  adesto_flash_send_address(address_to_write);
  ssp2_lab_exchange_byte(dummy_byte);
  read_data = ssp2_lab_exchange_byte(dummy_byte);
  adesto_ds();
  fprintf(stderr, "Now, read data '%X' successfully\n", read_data);
}

#endif
