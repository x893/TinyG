#ifndef __EEPROM_DRIVER_H
#define __EEPROM_DRIVER_H

#include <avr/io.h>
#include <avr/interrupt.h>

#include "xboot.h"

#ifndef EEPROM_PAGE_SIZE
	#define EEPROM_PAGE_SIZE E2PAGESIZE
#endif

uint8_t EEPROM_read_byte(uint16_t addr);
void EEPROM_write_byte(uint16_t addr, uint8_t byte);
uint16_t EEPROM_read_block(uint16_t addr, uint8_t *dest, uint16_t len);
uint16_t EEPROM_write_block(uint16_t addr, const uint8_t *src, uint16_t len);

void EEPROM_erase_page(uint16_t addr);
void EEPROM_erase_all(void);

#endif // __EEPROM_DRIVER_H
