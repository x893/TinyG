#include "uart.h"

void USART_Init(PORT_t * port, USART_t * device, uint8_t tx_pin, register8_t * rx_ctrl, uint16_t baud, uint8_t scale, uint8_t clk2x)
{
	port->DIRSET = (1 << tx_pin);
	device->BAUDCTRLA = (baud & USART_BSEL_gm);
	device->BAUDCTRLB = ((scale << USART_BSCALE_gp) & USART_BSCALE_gm) | ((baud >> 8) & ~USART_BSCALE_gm);
	if (clk2x)
		device->CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_CLK2X_bm;
	else
		device->CTRLB = USART_RXEN_bm | USART_TXEN_bm;
	*rx_ctrl = 0x18;
}

void USART_DeInit(PORT_t * port, USART_t * device, uint8_t tx_pin, register8_t * rx_ctrl)
{
	device->CTRLB = 0;
	device->BAUDCTRLA = 0;
	device->BAUDCTRLB = 0;
	port->DIRCLR = (1 << UART_TX_PIN);
	*rx_ctrl = 0;
}

void USART_Send(USART_t * device, unsigned char ch)
{
	device->DATA = ch;
	while (!(device->STATUS & USART_TXCIF_bm))
	{ }
	device->STATUS |= USART_TXCIF_bm;
}

void USART_Send_Hex(USART_t * device, uint8_t x)
{
	unsigned char c;
	c = x >> 4;
	if (c > 9)
		c += ('A' - 10);
	else
		c += '0';
	USART_Send(device, c);
	c = x & 0x0F;
	if (c > 9)
	c += ('A' - 10);
	else
	c += '0';
	USART_Send(device, c);
}

void USART_Send_Hex2(USART_t * device, uint16_t x)
{
	USART_Send_Hex(device, x >> 8);
	USART_Send_Hex(device, x);
}

void USART_Send_Hex4(USART_t * device, uint32_t x)
{
	USART_Send_Hex2(device, x >> 16);
	USART_Send_Hex2(device, x);
}
void USART_Send_B(USART_t * device, PGM_P s)
{
	uint8_t c;
	uint32_t addr = (uint32_t)s;
	addr += BOOT_SECTION_START;
	while ((c = Flash_ReadByte(addr++)) != 0)
	{
		USART_Send(device, c);
	}
}

void USARTC0_Init(uint16_t baud)
{
	USART_Init(&PORTC, &USARTC0, 3, &PORTC.PIN2CTRL, baud, 0, 0);
}
void USARTC1_Init(uint16_t baud)
{
	USART_Init(&PORTC, &USARTC1, 7, &PORTC.PIN6CTRL, baud, 0, 0);
}
void USARTC0_DeInit()
{
	USART_DeInit(&PORTC, &USARTC0, 3, &PORTC.PIN2CTRL);
}
void USARTC1_DeInit()
{
	USART_DeInit(&PORTC, &USARTC1, 7, &PORTC.PIN6CTRL);
}
void USARTC0_Send(unsigned char c)
{
	USART_Send(&USARTC0, c);
}
void USARTC1_Send(unsigned char c)
{
	USART_Send(&USARTC1, c);
}
void USARTC0_Send_B(const char * s)
{
	USART_Send_B(&USARTC0, s);
}
