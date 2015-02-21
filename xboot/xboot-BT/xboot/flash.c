#include "flash.h"

void Flash_ProgramPage(uint32_t page, uint8_t *buf, uint8_t erase)
{
	Flash_LoadFlashPage(buf);
	if (erase)
	{
		Flash_EraseWriteApplicationPage(page);
	}
	else
	{
		Flash_WriteApplicationPage(page);
	}
	Flash_WaitForSPM();
}
