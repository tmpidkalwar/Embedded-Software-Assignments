/**
 * @file ssp2_lab.h
 * @author Tejas Pidkalwar
 * @date 1st Oct 2020
 * @brief This file contains SSP2 driver configuration APIs
 */

#ifndef _SSP2_LAB_
#define _SSP2_LAB_

void ssp2_lab_init(uint32_t max_clock_mhz);

uint8_t ssp2_lab_exchange_byte(uint8_t data_out);

void ssp2_configure_pin_functions(void);

#endif