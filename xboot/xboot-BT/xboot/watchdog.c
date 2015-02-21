#include "watchdog.h"

void WDT_EnableAndSetTimeout( void )
{
        uint8_t temp = WDT_ENABLE_bm | WDT_CEN_bm | WATCHDOG_TIMEOUT;
        CCP = CCP_IOREG_gc;
        WDT.CTRL = temp;
        
        /* Wait for WD to synchronize with new settings. */
        while(WDT_IsSyncBusy());
}

void WDT_Disable( void )
{
        uint8_t temp = (WDT.CTRL & ~WDT_ENABLE_bm) | WDT_CEN_bm;
        CCP = CCP_IOREG_gc;
        WDT.CTRL = temp;
}
