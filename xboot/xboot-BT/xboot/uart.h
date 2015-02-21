#ifndef __UART_H
#define __UART_H

#include "xboot.h"
#include <avr/pgmspace.h>

// nonzero if character has been received
#define uart_char_received()	(UART_DEVICE.STATUS & USART_RXCIF_bm)
// current character in UART receive buffer
#define uart_cur_char()			UART_DEVICE.DATA


extern void USART_Init(PORT_t * port, USART_t * device, uint8_t tx_pin, register8_t * rx_ctrl, uint16_t baud, uint8_t scale, uint8_t clk2x);
extern void USART_DeInit(PORT_t * port, USART_t * device, uint8_t tx_pin, register8_t * rx_ctrl);
extern void USART_Send(USART_t * device, unsigned char c);
extern void USART_Send_B(USART_t * device, PGM_P s);
void USART_Send_Hex4(USART_t * device, uint32_t x);
void USART_Send_Hex2(USART_t * device, uint16_t x);

extern void USARTC0_Init(uint16_t baud);
extern void USARTC1_Init(uint16_t baud);
extern void USARTC0_DeInit(void);
extern void USARTC1_DeInit(void);
extern void USARTC0_Send(unsigned char c);
extern void USARTC1_Send(unsigned char c);
extern void USARTC0_Send_B(const char * s);

#endif // __UART_H
