// XBoot config header file
// MCU: atxmega256a3
// F_CPU: 32000000

#define LED_PIN			5
#define LED_PORT		PORTA
#define LED_ON()
//	LED_PORT.OUTSET = (1 << LED_PIN)
#define LED_OFF()
//	LED_PORT.OUTCLR = (1 << LED_PIN)
#define LED_TOGGLE()
//	LED_PORT.OUTTGL = (1 << LED_PIN)
#define LED_INIT()
/*	do {							\
		LED_ON();					\
		LED_PORT.DIRSET = (1 << LED_PIN);	\
	} while(0)
*/
#define LED_ON_DELAY		500000UL
#define LED_OFF_DELAY		540000UL
#define XB_BOOT_DELAY		7

#define UART_BAUD_RATE		115200

#if (UART_NUMBER == 0)
	#define UART_RX_PIN		2
	#define UART_TX_PIN		3
#else
	#define UART_RX_PIN		6
	#define UART_TX_PIN		7
#endif

#define UART_PORT			PORTC
#define UART_DEVICE_PORT	C0
#define UART_DEVICE			USARTC0
#define UART_RX_PIN_CTRL	PORTC.PIN2CTRL
#define UART_TX_PIN_CTRL	PORTC.PIN3CTRL

#define UART_INIT(b)		USARTC0_Init(b)
#define UART_DEINIT()		USARTC0_DeInit()
#define UART_Send(c)		USARTC0_Send(c)
#define UART_Send_B(s)		USARTC0_Send_B(PSTR(s))
#define UART_Send_Hex4(d)	USART_Send_Hex4(&UART_DEVICE, d)
#define UART_Send_Hex2(d)	USART_Send_Hex2(&UART_DEVICE, d)

#define WATCHDOG_TIMEOUT	WDT_PER_1KCLK_gc
