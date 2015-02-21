#include "xboot.h"

unsigned char Buffer[SPM_PAGESIZE];
ADDR_T address;
uint16_t data;
uint8_t in_bootloader;
uint8_t val;
uint32_t led_delay;

typedef struct FirmwareInfo_s {
	uint32_t	Size;
	uint16_t	CRC16;
	} FirmwareInfo_t;

#define FWINFO_START	(APP_SECTION_START + APP_SECTION_SIZE - APP_SECTION_PAGE_SIZE)
uint8_t CheckAppImage(void);
void WriteAppChecksum(uint32_t size);

// Main code
int main(void)
{
	WDT_Disable();

	#ifdef USE_32MHZ_RC
		#if (F_CPU != 32000000L)
			#error F_CPU must match oscillator setting!
		#endif // F_CPU

		OSC.CTRL |= OSC_RC32MEN_bm;						// turn on 32 MHz oscillator
		while (!(OSC.STATUS & OSC_RC32MRDY_bm)) { };	// wait for it to start
		CCP = CCP_IOREG_gc;
		CLK.CTRL = CLK_SCLKSEL_RC32M_gc;

		#ifdef USE_DFLL
			DFLLRC32M.CTRL = DFLL_ENABLE_bm;
		#endif // USE_DFLL
	#else // USE_32MHZ_RC
		#if (F_CPU != 2000000L)
			#error F_CPU must match oscillator setting!
		#endif // F_CPU
		#ifdef USE_DFLL
			DFLLRC2M.CTRL = DFLL_ENABLE_bm;
		#endif // USE_DFLL
	#endif // USE_32MHZ_RC

	LED_INIT();
	// Bluetooth initialization 
	UART_INIT(USART_BAUD_9600);
	UART_Send_B("AT+BAUD8");		// Set speed 115200
	_delay_ms(1000);
	UART_INIT(USART_BAUD_115200);	// Change USART to 115200
	UART_Send_B("AT+NAMETinyG");	// Set BT device name
	_delay_ms(1000);
	UART_Send_B("AT+PIN1963");		// Set BT password
	_delay_ms(1000);

	// Correct CRC, enable auto jump to application
	in_bootloader = CheckAppImage();
	//	in_bootloader == 1	CRC not correct, never jump to application
	//	in_bootloader == 2	CRC correct, after small delay go to application
	data = 0;
	address = 0;
	led_delay = 0;
	while (in_bootloader)	// Main bootloader
	{
		val = get_char_nowait();
		if (val == 0)
		{
			led_delay++;
			if (led_delay == LED_ON_DELAY)
			{
				LED_ON();
			}
			else if (led_delay == LED_OFF_DELAY)
			{
				LED_OFF();
				led_delay = 0;
				if (in_bootloader == XB_BOOT_DELAY)
				{	// Jump to application
					in_bootloader = 0;
					break;
				}					
				else if (in_bootloader > 1)
				{	// Correct CRC, increment boot delay
					in_bootloader++;
				}					
			}
			continue;
		}
		led_delay = 0;
		LED_ON();

		in_bootloader = 1;	// Disable auto start application
		if (val == CMD_CHECK_AUTOINCREMENT)
		{
			UART_Send(REPLY_YES);
		}
		else if (val == CMD_SET_ADDRESS)
		{	// Set address
			address = get_2bytes();	// Read address high then low
			UART_Send(REPLY_ACK);
		}
		else if (val == CMD_SET_EXT_ADDRESS)
		{	// Extended address
			// Read address high then low
			address = get_3bytes();
			UART_Send(REPLY_ACK);
		}
		else if (val == CMD_CHIP_ERASE)
		{	// Chip erase
			Flash_EraseApplicationSection();	// Erase the application section
			SP_WaitForSPM();					// Wait for completion
			EEPROM_erase_all();					// Erase EEPROM
			UART_Send(REPLY_ACK);
		}
		else if (val == CMD_CHECK_BLOCK_SUPPORT )
		{	// Check block load support
			UART_Send(REPLY_YES);
			UART_Send((SPM_PAGESIZE >> 8) & 0xFF);	// Send block size (page size)
			UART_Send(SPM_PAGESIZE & 0xFF);
		}
		else if (val == CMD_BLOCK_LOAD)
		{	// Block load
			int bsize = get_2bytes();	// Block size
			val = get_char();			// Memory type
			UART_Send(BlockLoad(bsize, val, &address));
		}
		else if (val == CMD_BLOCK_READ)
		{	// Block read
			int bsize = get_2bytes();	// Block size
			val = get_char();			// Memory type
			BlockRead(bsize, val, &address);
		}
		else if (val == CMD_READ_BYTE)
		{	// Read program memory byte
			unsigned int w = Flash_ReadWord((address << 1));
			UART_Send(w >> 8);
			UART_Send(w);
			address++;
		}
		else if (val == CMD_WRITE_LOW_BYTE)
		{	// Write program memory low byte
			data = get_char();	// get low byte
			UART_Send(REPLY_ACK);
		}
		else if (val == CMD_WRITE_HIGH_BYTE)
		{	// Write program memory high byte
			data |= (get_char() << 8);		// get high byte; combine
			Flash_LoadFlashWord((address << 1), data);
			address++;
			UART_Send(REPLY_ACK);
		}
		else if (val == CMD_WRITE_PAGE)
		{	// Write page
			if (address >= (APP_SECTION_SIZE >> 1))
			{	// don't allow bootloader overwrite
				UART_Send(REPLY_ERROR);
			}
			else
			{
				Flash_WriteApplicationPage( address << 1);
				UART_Send(REPLY_ACK);
			}
		}
		else if (val == CMD_WRITE_EEPROM_BYTE)
		{	// Write EEPROM memory
			EEPROM_write_byte(address, get_char());
			address++;
		}
		else if (val == CMD_READ_EEPROM_BYTE)
		{	// Read EEPROM memory
			char c = EEPROM_read_byte(address);
			UART_Send(c);
			address++;
		}
		else if (val == CMD_WRITE_LOCK_BITS)
		{	// Write lockbits
			SP_WriteLockBits( get_char() );
			UART_Send(REPLY_ACK);
		}
		else if (val == CMD_READ_LOCK_BITS)
		{	// Read lockbits
			UART_Send(SP_ReadLockBits());
		}
		else if (val == CMD_READ_LOW_FUSE_BITS)
		{	// Read low fuse bits
			UART_Send(SP_ReadFuseByte(0));
		}
		else if (val == CMD_READ_HIGH_FUSE_BITS)
		{	// Read high fuse bits
			UART_Send(SP_ReadFuseByte(1));
		}
		else if (val == CMD_READ_EXT_FUSE_BITS)
		{	// Read extended fuse bits
			UART_Send(SP_ReadFuseByte(2));
		}
		// Enter and leave programming mode
		else if ((val == CMD_ENTER_PROG_MODE) || (val == CMD_LEAVE_PROG_MODE))
		{	// just acknowledge
			UART_Send(REPLY_ACK);
		}
		else if (val == CMD_EXIT_BOOTLOADER)
		{	// Exit bootloader
			if (CheckAppImage() == 2)
			{
				in_bootloader = 0;
				UART_Send(REPLY_ACK);
			}
			else
				UART_Send(REPLY_ERROR);
		}
		else if (val == CMD_PROGRAMMER_TYPE)
		{	// Get programmer type
			UART_Send('S');
		}
		else if (val == CMD_DEVICE_CODE)
		{	// Return supported device codes
			// send only this device
			UART_Send(123); // TODO
			UART_Send(0);
		}
		else if ((val == CMD_SET_LED) || (val == CMD_CLEAR_LED) || (val == CMD_SET_TYPE))
		{	// Set LED, clear LED, and set device type
			get_char();	// discard parameter
			UART_Send(REPLY_ACK);
		}
		else if (val == CMD_PROGRAM_ID)
		{	// Return program identifier
			UART_Send('X');
			UART_Send('B');
			UART_Send('o');
			UART_Send('o');
			UART_Send('t');
			UART_Send('+');
			UART_Send('+');
		}
		else if (val == CMD_VERSION)
		{	// Read software version
			UART_Send('0' + XBOOT_VERSION_MAJOR);
			UART_Send('0' + XBOOT_VERSION_MINOR);
		}
		else if (val == CMD_READ_SIGNATURE)
		{	// Read signature bytes
			UART_Send(SIGNATURE_2);
			UART_Send(SIGNATURE_1);
			UART_Send(SIGNATURE_0);
		}
		//	hA
		else if (val == CMD_CRC)
		{
			uint32_t start = 0;
			uint32_t length = 0;
			uint16_t crc;

			val = get_char();

			switch (val)
			{
				case SECTION_FLASH:
					length = PROGMEM_SIZE;
					break;
				case SECTION_APPLICATION:
					length = APP_SECTION_SIZE;
					break;
				case SECTION_BOOT:
					start = BOOT_SECTION_START;
					length = BOOT_SECTION_SIZE;
					break;
				case SECTION_CUSTOM:
					start = address;	// Use Set extended address
					length = get_3bytes();
					break;
			}
			if (length != 0)
			{
				crc = crc16_block(start, length);
				UART_Send(REPLY_ACK);
				UART_Send((crc >> 8) & 0xFF);
				UART_Send(crc & 0xFF);
			}
			else
			{
				UART_Send(REPLY_ERROR);
				UART_Send(0);
				UART_Send(0);
			}
		}
		else if (val == CMD_CRC_WRITE)
		{
			WriteAppChecksum(get_3bytes());
			UART_Send(REPLY_ACK);
		}
		else if (val != CMD_SYNC)
		{
			UART_Send(REPLY_ACK);
		}

		// Wait for any lingering SPM instructions to finish
		Flash_WaitForSPM();
		LED_OFF();
	}

	// Bootloader exit section
	// Code here runs after the bootloader has exited,
	// but before the application code has started
	// --------------------------------------------------

	UART_DEINIT();
	LED_ON();

	asm("jmp 0");	// Jump into main code

#ifdef __builtin_unreachable
	// Size optimization as the asm jmp will not return
	// However, it seems it is not available on older versions of gcc
	__builtin_unreachable();
#endif
}

unsigned char __attribute__ ((noinline)) get_char(void)
{
	while (!uart_char_received());
	return uart_cur_char();
}

unsigned char __attribute__ ((noinline)) get_char_nowait(void)
{
	if (uart_char_received())
		return uart_cur_char();
	return 0;
}

uint32_t __attribute__ ((noinline)) get_3bytes()
{
	uint32_t val = ((uint32_t)get_char() << 16);
	return  val | (uint32_t)get_2bytes();
}

uint16_t __attribute__ ((noinline)) get_2bytes()
{
	uint16_t val = ((uint16_t)get_char() << 8);
	return  val | (uint16_t)get_char();
}

void clear_buffer(void)
{
	unsigned char *ptr = Buffer;
	for (long i = 0; i < SPM_PAGESIZE; i++)
		*(ptr++) = 0xFF;
}

unsigned char BlockLoad(unsigned int size, unsigned char mem, ADDR_T *address)
{
	ADDR_T tempaddress;

	// fill up Buffer
	for (int i = 0; i < SPM_PAGESIZE; i++)
	{
		char c = 0xFF;
		if (i < size)
			c = get_char();
		Buffer[i] = c;
	}

	// EEPROM memory type.
	if(mem == MEM_EEPROM)
	{
		EEPROM_write_block(*address, Buffer, size);
		(*address) += size;
		return REPLY_ACK; // Report programming OK
	}

	else if (mem == MEM_FLASH || mem == MEM_USERSIG)
	{	// NOTE: For flash programming, 'address' is given in words.
		tempaddress = (*address) << 1;  // Store address in page.
		(*address) += size >> 1;
		if (mem == MEM_FLASH)
		{
			Flash_ProgramPage(tempaddress, Buffer, 1);
		}
		else if (mem == MEM_USERSIG)
		{
			Flash_LoadFlashPage(Buffer);
			Flash_EraseUserSignatureRow();
			Flash_WaitForSPM();
			Flash_WriteUserSignatureRow();
			Flash_WaitForSPM();
		}
		return REPLY_ACK;
	}
	else
		return REPLY_ERROR;
}

void BlockRead(unsigned int size, unsigned char mem, ADDR_T *address)
{
	int offset = 0;
	int size2 = size;
        
	// EEPROM memory type.
        
	if (mem == MEM_EEPROM) // Read EEPROM
	{
		EEPROM_read_block(*address, Buffer, size);
		(*address) += size;
	}
	// Flash memory type.
	else if (mem == MEM_FLASH || mem == MEM_USERSIG || mem == MEM_PRODSIG)
	{
		(*address) <<= 1; // Convert address to bytes temporarily.
		do
		{
			if (mem == MEM_FLASH)
				Buffer[offset++] = Flash_ReadByte(*address);
			else if (mem == MEM_USERSIG)
				Buffer[offset++] = SP_ReadUserSignatureByte(*address);
			else if (mem == MEM_PRODSIG)
				Buffer[offset++] = SP_ReadCalibrationByte(*address);
			Flash_WaitForSPM();
			(*address)++;    // Select next word in memory.
			size--;          // Subtract two bytes from number of bytes to read
		} while (size);         // Repeat until all block has been read
		(*address) >>= 1;       // Convert address back to Flash words again.
	}
	else
		// bad memory type
		return;

	// send bytes
	for (int i = 0; i < size2; i++)
		UART_Send(Buffer[i]);
}

uint16_t crc16_block(uint32_t start, uint32_t length)
{
	uint16_t crc = 0;
	int bc = SPM_PAGESIZE;

	for ( ; length > 0; length--)
	{
		if (bc == SPM_PAGESIZE)
		{
			Flash_ReadFlashPage(Buffer, start);
			start += SPM_PAGESIZE;
			bc = 0;
		}
		crc = _crc16_update(crc, Buffer[bc]);
		bc++;
	}
	return crc;
}

uint8_t CheckAppImage(void)
{
	uint32_t app_size;
	uint16_t app_crc;
	FirmwareInfo_t *fw_info = (FirmwareInfo_t *)&Buffer[0];

	Flash_ReadFlashPage(Buffer, FWINFO_START);
	app_size = fw_info->Size;
	app_crc = fw_info->CRC16;
	if (app_size != 0xFFFFFFFF)
	{
		data = crc16_block(APP_SECTION_START, fw_info->Size);
		if (app_crc == data)
			return 2;
	}
	return 1;
}

void WriteAppChecksum(uint32_t size)
{
	FirmwareInfo_t *fw_info = (FirmwareInfo_t *)&Buffer[0];

	data = crc16_block(APP_SECTION_START, size);
	Flash_ReadFlashPage(Buffer, FWINFO_START);
	if (size != fw_info->Size || data != fw_info->Size)
	{
		for (int i = 0; i < APP_SECTION_PAGE_SIZE; i++)
			Buffer[i] = 0xFF;

		fw_info->Size = size;
		fw_info->CRC16 = data;
		Flash_ProgramPage(FWINFO_START, Buffer, 1);
		Flash_WaitForSPM();
	}
}

