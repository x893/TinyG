#ifndef __XBOOT_H
#define __XBOOT_H

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/crc16.h>

// Version
#define XBOOT_VERSION_MAJOR 1
#define XBOOT_VERSION_MINOR 7

// config.h
#ifdef USE_CONFIG_H
	#include "config.h"
#endif // USE_CONFIG_H

// DFLL for better stability
#define USE_DFLL
// use 32MHz osc if makefile calls for it
#if (F_CPU == 32000000L)
	// defaults to 2MHz RC oscillator
	// define USE_32MHZ_RC to override
	#define USE_32MHZ_RC
#endif // F_CPU

// BAUD Rate Values
#if (F_CPU == 32000000UL)								// Known good at 32MHz
	#define USART_BAUD_9600		207
	#define USART_BAUD_115200	16
#elif (F_CPU == 2000000L) && (UART_BAUD_RATE == 19200)	// Known good at 2MHz
	#define UART_BSEL_VALUE         12
	#define UART_BSCALE_VALUE       0
	#define UART_CLK2X              1
#elif (F_CPU == 2000000L) && (UART_BAUD_RATE == 38400)
	#define UART_BSEL_VALUE         22
	#define UART_BSCALE_VALUE       -2
	#define UART_CLK2X              1
#elif (F_CPU == 2000000L) && (UART_BAUD_RATE == 57600)
	#define UART_BSEL_VALUE         26
	#define UART_BSCALE_VALUE       -3
	#define UART_CLK2X              1
#elif (F_CPU == 2000000L) && (UART_BAUD_RATE == 115200)
	#define UART_BSEL_VALUE         19
	#define UART_BSCALE_VALUE       -4
	#define UART_CLK2X              1
// Known good at 32MHz
#elif (F_CPU == 32000000L) && (UART_BAUD_RATE == 19200)
	#define UART_BSEL_VALUE         103
	#define UART_BSCALE_VALUE       0
	#define UART_CLK2X              0
#elif (F_CPU == 32000000L) && (UART_BAUD_RATE == 38400)
	#define UART_BSEL_VALUE         51
	#define UART_BSCALE_VALUE       0
	#define UART_CLK2X              0
#elif (F_CPU == 32000000L) && (UART_BAUD_RATE == 57600)
	#define UART_BSEL_VALUE         34
	#define UART_BSCALE_VALUE       0
	#define UART_CLK2X              0
#elif (F_CPU == 32000000L) && (UART_BAUD_RATE == 115200)
	#define UART_BSEL_VALUE         16
	#define UART_BSCALE_VALUE       0
	#define UART_CLK2X              0
// None of the above, so calculate something
#else
	#warning Not using predefined BAUD rate, possible BAUD rate error!
	#if (F_CPU == 2000000L)
		#define UART_BSEL_VALUE         ((F_CPU) / ((uint32_t)UART_BAUD_RATE * 8) - 1)
		#define UART_BSCALE_VALUE       0
		#define UART_CLK2X              1
	#else
		#define UART_BSEL_VALUE         ((F_CPU) / ((uint32_t)UART_BAUD_RATE * 16) - 1)
		#define UART_BSCALE_VALUE       0
		#define UART_CLK2X              0
	#endif
#endif

#ifndef EEPROM_PAGE_SIZE
	#define EEPROM_PAGE_SIZE E2PAGESIZE
#endif

#ifndef EEPROM_BYTE_ADDRESS_MASK
	#if EEPROM_PAGE_SIZE == 32
		#define EEPROM_BYTE_ADDRESS_MASK 0x1f
	#elif EEPROM_PAGE_SIZE == 16
		#define EEPROM_BYTE_ADDRESS_MASK = 0x0f
	#elif EEPROM_PAGE_SIZE == 8
		#define EEPROM_BYTE_ADDRESS_MASK = 0x07
	#elif EEPROM_PAGE_SIZE == 4
		#define EEPROM_BYTE_ADDRESS_MASK = 0x03
	#else
		#error Unknown EEPROM page size!  Please add new byte address value!
	#endif
#endif

typedef uint32_t ADDR_T;

#include "protocol.h"
#include "flash.h"
#include "eeprom_driver.h"
#include "uart.h"
#include "watchdog.h"

unsigned char __attribute__ ((noinline)) ow_slave_read_bit(void);
void __attribute__ ((noinline)) ow_slave_write_bit(unsigned char b);
void ow_slave_wait_bit(void);

unsigned char __attribute__ ((noinline)) get_char(void);
unsigned char __attribute__ ((noinline)) get_char_nowait(void);
void __attribute__ ((noinline)) send_char(unsigned char c);
uint16_t __attribute__ ((noinline)) get_2bytes(void);
uint32_t __attribute__ ((noinline)) get_3bytes(void);

void clear_buffer(void);

unsigned char BlockLoad(unsigned int size, unsigned char mem, ADDR_T *address);
void BlockRead(unsigned int size, unsigned char mem, ADDR_T *address);

uint16_t crc16_block(uint32_t start, uint32_t length);
void install_firmware(void);

#endif // __XBOOT_H
