// File =  atmega2560_bootloader.hex
// Loader start: 3E000, length: 8192
// MD5 sum = 12 AA 80 07 4D 74 E3 DA BF 2D 25 84 6D 99 F7 20
// Pfft! See: http://forum.arduino.cc/index.php?topic=204174
// Date: 15 December 2013
// Mods to bootloader by Nick Gammon

// In particular, the watchdog part of the bootloader changed to read:

/*
#include <avr/wdt.h>

...


#ifdef _FIX_ISSUE_181_
  //************************************************************************
  //* Dec 29, 2011  <MLS> Issue #181, added watch dog timmer support
  //* handle the watch dog timer
  uint8_t mcuStatusReg;
  mcuStatusReg  = MCUSR;

  __asm__ __volatile__ ("cli");
  __asm__ __volatile__ ("wdr");
  MCUSR = 0;
  WDTCSR  |=  _BV(WDCE) | _BV(WDE);
  WDTCSR  = 0;
  wdt_disable ();

  __asm__ __volatile__ ("sei");
  // check if WDT generated the reset, if so, go straight to app
  if (mcuStatusReg & _BV(WDRF))
    {

    boot_rww_enable();        // enable application section


    asm volatile(
                 "clr r30   \n\t"
                 "clr r31   \n\t"
                 "ijmp  \n\t"
                 );

    }
  //************************************************************************
#endif

*/

const byte testcode_hex [] PROGMEM = {
 
}; // end of atmega2560_bootloader_hex

