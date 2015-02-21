#include "eeprom_driver.h"
#include "string.h"

// NVM call
static inline void NVM_EXEC(void)
{
	void *z = (void *)&NVM_CTRLA;
        
	__asm__ volatile("out %[ccp], %[ioreg]"  "\n\t"
		"st z, %[cmdex]"
		:
		: [ccp] "I" (_SFR_IO_ADDR(CCP)),
		[ioreg] "d" (CCP_IOREG_gc),
		[cmdex] "r" (NVM_CMDEX_bm),
		[z] "z" (z)
	);
}

#define NVM_EXEC_WRAPPER NVM_EXEC

void wait_for_nvm(void)
{
	while (NVM.STATUS & NVM_NVMBUSY_bm) { };
}

void flush_buffer(void)
{
	wait_for_nvm();
	if ((NVM.STATUS & NVM_EELOAD_bm) != 0)
	{
		NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
		NVM_EXEC();
	}
}

uint8_t EEPROM_read_byte(uint16_t addr)
{
	wait_for_nvm();

	NVM.ADDR0 = addr & 0xFF;
	NVM.ADDR1 = (addr >> 8) & 0x1F;
	NVM.ADDR2 = 0;

	NVM.CMD = NVM_CMD_READ_EEPROM_gc;
	NVM_EXEC();

	return NVM.DATA0;
}


void EEPROM_write_byte(uint16_t addr, uint8_t byte)
{
	flush_buffer();
	NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

	NVM.ADDR0 = addr & 0xFF;
	NVM.ADDR1 = (addr >> 8) & 0x1F;
	NVM.ADDR2 = 0;

	NVM.DATA0 = byte;

	NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
	NVM_EXEC_WRAPPER();
}


uint16_t EEPROM_read_block(uint16_t addr, uint8_t *dest, uint16_t len)
{
	uint16_t cnt = 0;

	NVM.ADDR2 = 0;

	wait_for_nvm();

	while (len > 0)
	{
		NVM.ADDR0 = addr & 0xFF;
		NVM.ADDR1 = (addr >> 8) & 0x1F;

		NVM.CMD = NVM_CMD_READ_EEPROM_gc;
		NVM_EXEC();

		*(dest++) = NVM.DATA0; addr++;

		len--; cnt++;
	}
	return cnt;
}


uint16_t EEPROM_write_block(uint16_t addr, const uint8_t *src, uint16_t len)
{
	uint8_t byte_addr = addr % EEPROM_PAGE_SIZE;
	uint16_t page_addr = addr - byte_addr;
	uint16_t cnt = 0;

	flush_buffer();
	wait_for_nvm();
	NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

	NVM.ADDR1 = 0;
	NVM.ADDR2 = 0;

	while (len > 0)
	{
		NVM.ADDR0 = byte_addr;

		NVM.DATA0 = *(src++);
                
		byte_addr++;
		len--;
                
		if (len == 0 || byte_addr >= EEPROM_PAGE_SIZE)
		{
			NVM.ADDR0 = page_addr & 0xFF;
			NVM.ADDR1 = (page_addr >> 8) & 0x1F;
                        
			NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
			NVM_EXEC();
                        
			page_addr += EEPROM_PAGE_SIZE;
			byte_addr = 0;

			wait_for_nvm();

			NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;
		}
		cnt++;
	}

	return cnt;
}


void EEPROM_erase_page(uint16_t addr)
{
	NVM.ADDR0 = addr & 0xFF;
	NVM.ADDR1 = (addr >> 8) & 0x1F;
	NVM.ADDR2 = 0;

	wait_for_nvm();

	NVM.CMD = NVM_CMD_ERASE_EEPROM_PAGE_gc;
	NVM_EXEC_WRAPPER();
}


void EEPROM_erase_all(void)
{
	wait_for_nvm();

	NVM.CMD = NVM_CMD_ERASE_EEPROM_gc;
	NVM_EXEC_WRAPPER();
}
