

#define VERSION "1.30"
const int ENTER_PROGRAMMING_ATTEMPTS = 50;
#include <SPI.h>
#include <avr/pgmspace.h>
const unsigned long BAUD_RATE = 115200;
//******************************************************************************
byte Lfuse = 0xEF;
byte Hfuse = 0xD9;
byte Efuse = 0xF4;
byte Lockbit = 0xCF;
long int Saddr = 0x00;
long int CODEaddr = 0x00;
const byte RESET = 6;  // --> goes to reset on the target board
int blue = 9;
int red = 13;
//*************************************************************************************
#define USE_ATMEGA8 true
#define USE_ATMEGA16U2 true    // Uno USB interface chip
#define USE_ATMEGA32U4 true    // Leonardo
#define USE_ATMEGA168 true
#define USE_ATMEGA1280 true
#define USE_ATMEGA1284 true
#define USE_ATMEGA2560 true
//const byte CLOCKOUT = 9;
const byte CLOCKOUT = 52;
int error = 0;
#if ARDUINO < 100
const byte SCK = 13;    // SPI clock
#endif

// number of items in an array
#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))

// programming commands to send via SPI to the chip
enum {
  progamEnable = 0xAC,
  // writes are preceded by progamEnable
  chipErase = 0x80,
  writeLockByte = 0xE0,
  writeLowFuseByte = 0xA0,
  writeHighFuseByte = 0xA8,
  writeExtendedFuseByte = 0xA4,
  pollReady = 0xF0,
  programAcknowledge = 0x53,
  readSignatureByte = 0x30,
  readCalibrationByte = 0x38,
  readLowFuseByte = 0x50,       readLowFuseByteArg2 = 0x00,
  readExtendedFuseByte = 0x50,  readExtendedFuseByteArg2 = 0x08,
  readHighFuseByte = 0x58,      readHighFuseByteArg2 = 0x08,
  readLockByte = 0x58,          readLockByteArg2 = 0x00,
  readProgramMemory = 0x20,
  writeProgramMemory = 0x4C,
  loadExtendedAddressByte = 0x4D,
  loadProgramMemory = 0x40,
};  // end of enum

// structure to hold signature and other relevant data about each chip
typedef struct {
  byte sig [3];
  char * desc;
  unsigned long flashSize;
  unsigned int baseBootSize;
  const byte * bootloader;
  unsigned long loaderStart;  // bytes
  unsigned int loaderLength;  // bytes
  unsigned long pageSize;     // bytes
  byte lowFuse, highFuse, extFuse, lockByte;
  const byte * code;
  unsigned long codeStart;  // bytes
  unsigned long codeLength;  // bytes
  byte timedWrites;    // if pollUntilReady won't work by polling the chip
} signatureType;

const unsigned long kb = 1024;
#include "bootloader.h"
#include "testcode.h"
signatureType signatures [] =
{
  { { 0x1E, 0x91, 0x0B }, "ATtiny24",   2 * kb,         0 },
  { { 0x1E, 0x92, 0x07 }, "ATtiny44",   4 * kb,         0 },
  { { 0x1E, 0x93, 0x0C }, "ATtiny84",   8 * kb,         0 },

  // Attiny85 family
  { { 0x1E, 0x91, 0x08 }, "ATtiny25",   2 * kb,         0 },
  { { 0x1E, 0x92, 0x06 }, "ATtiny45",   4 * kb,         0 },
  { { 0x1E, 0x93, 0x0B }, "ATtiny85",   8 * kb,         0 },

  // Atmega328 family
  { { 0x1E, 0x92, 0x0A }, "ATmega48PA",   4 * kb,         0 },
  { { 0x1E, 0x93, 0x0F }, "ATmega88PA",   8 * kb,       256 },
  { { 0x1E, 0x94, 0x0B }, "ATmega168PA", 16 * kb,       256,
#if USE_ATMEGA168
    bootloader_hex,   // loader image
#else
    0,
#endif
    Saddr,               // start address
#if USE_ATMEGA168
    sizeof bootloader_hex,
#else
    0,
#endif
    128,          // page size in bytes (for committing)
    Lfuse,         // fuse low byte: external full-swing crystal
    Hfuse,         // fuse high byte: SPI enable, brown-out detection at 2.7V
    Efuse,         // fuse extended byte: boot into bootloader, 512 byte bootloader
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex
  },       // lock bits: SPM is not allowed to write to the Boot Loader section.

  { { 0x1E, 0x95, 0x0F }, "ATmega328P",  32 * kb,       512,
    bootloader_hex,   // loader image
    Saddr,               // start address
    sizeof bootloader_hex,
    128,          // page size in bytes (for committing)
    Lfuse,         // fuse low byte: external clock, max start-up time
    Hfuse,         // fuse high byte: SPI enable, boot into bootloader, 512 byte bootloader
    Efuse,         // fuse extended byte: brown-out detection at 2.7V
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex
  },       // lock bits: SPM is not allowed to write to the Boot Loader section.

  { { 0x1E, 0x95, 0x14 }, "ATmega328",  32 * kb,       512,
    bootloader_hex,   // loader image
    Saddr,               // start address
    sizeof bootloader_hex,
    128,          // page size in bytes (for committing)
    Lfuse,         // fuse low byte: external clock, max start-up time
    Hfuse,         // fuse high byte: SPI enable, boot into bootloader, 512 byte bootloader
    Efuse,         // fuse extended byte: brown-out detection at 2.7V
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex
  },       // lock bits: SPM is not allowed to write to the Boot Loader section.

  // Atmega644 family
  { { 0x1E, 0x94, 0x0A }, "ATmega164P",   16 * kb,      256 },
  { { 0x1E, 0x95, 0x08 }, "ATmega324P",   32 * kb,      512 },
  { { 0x1E, 0x96, 0x0A }, "ATmega644P",   64 * kb,   1 * kb },

  // Atmega2560 family
  { { 0x1E, 0x96, 0x08 }, "ATmega640",    64 * kb,   1 * kb },
  { { 0x1E, 0x97, 0x03 }, "ATmega1280",  128 * kb,   1 * kb,
#if USE_ATMEGA1280
    bootloader_hex,
#else
    0,
#endif
    Saddr,      // start address
#if USE_ATMEGA1280
    sizeof bootloader_hex,
#else
    0,
#endif
    256,          // page size in bytes (for committing)
    Lfuse,         // fuse low byte: external clock, max start-up time
    Hfuse,         // fuse high byte: SPI enable, boot into bootloader, 1280 byte bootloader
    Efuse,         // fuse extended byte: brown-out detection at 2.7V
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex
  },       // lock bits: SPM is not allowed to write to the Boot Loader section.

  { { 0x1E, 0x97, 0x04 }, "ATmega1281",  128 * kb,   1 * kb },

  { { 0x1E, 0x98, 0x01 }, "ATmega2560",  256 * kb,   1 * kb,
#if USE_ATMEGA2560
    bootloader_hex,// loader image
#else
    0,
#endif
    Saddr,      // start address
#if USE_ATMEGA2560
    sizeof bootloader_hex,
#else
    0,
#endif
    256,          // page size in bytes (for committing)
    Lfuse,         // fuse low byte: external clock, max start-up time
    Hfuse,         // fuse high byte: SPI enable, boot into bootloader, 8192 byte bootloader
    Efuse,         // fuse extended byte: brown-out detection at 2.7V
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex
  },       // lock bits: SPM is not allowed to write to the Boot Loader section.


  { { 0x1E, 0x98, 0x02 }, "ATmega2561",  256 * kb,   1 * kb },

  // Atmega32U2 family
  { { 0x1E, 0x93, 0x89 }, "ATmega8U2",    8 * kb,   512 },
  { { 0x1E, 0x94, 0x89 }, "ATmega16U2",  16 * kb,   512,
#if USE_ATMEGA16U2
    bootloader_hex,// loader image
#else
    0,
#endif
    Saddr,      // start address
#if USE_ATMEGA16U2
    sizeof bootloader_hex,
#else
    0,
#endif
    128,          // page size in bytes (for committing)
    Lfuse,         // fuse low byte: external clock, m
    Hfuse,         // fuse high byte: SPI enable, NOT boot into bootloader, 4096 byte bootloader
    Efuse,         // fuse extended byte: brown-out detection at 2.6V
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex
  },       // lock bits

  { { 0x1E, 0x95, 0x8A }, "ATmega32U2",  32 * kb,   512,},

  // Atmega32U4 family
  { { 0x1E, 0x94, 0x88 }, "ATmega16U4",  16 * kb,   512 },
  { { 0x1E, 0x95, 0x87 }, "ATmega32U4",  32 * kb,   4 * kb,
#if USE_ATMEGA32U4
    bootloader_hex,// loader image
#else
    0,
#endif
    Saddr,      // start address
#if USE_ATMEGA32U4
    sizeof bootloader_hex,
#else
    0,
#endif
    128,          // page size in bytes (for committing) (datasheet is wrong about it being 128 words)
    Lfuse,         // fuse low byte: external clock, max start-up time
    Hfuse,         // fuse high byte: SPI enable, boot into bootloader, 1280 byte bootloader
    Efuse,         // fuse extended byte: brown-out detection at 2.6V
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex
  },       // lock bits: SPM is not allowed to write to the Boot Loader section.

  // ATmega1284P family
  { { 0x1E, 0x97, 0x05 }, "ATmega1284P", 128 * kb,   1 * kb,
#if USE_ATMEGA1284
    bootloader_hex,
#else
    0,
#endif
    Saddr,      // start address
#if USE_ATMEGA1284
    sizeof bootloader_hex,
#else
    0,
#endif
    256,          // page size in bytes (for committing)
    Lfuse,         // fuse low byte: external clock, max start-up time
    Hfuse,         // fuse high byte: SPI enable, boot into bootloader, 1024 byte bootloader
    Efuse,         // fuse extended byte: brown-out detection at 2.7V
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex
  },       // lock bits: SPM is not allowed to write to the Boot Loader section.

  // ATtiny4313 family
  { { 0x1E, 0x91, 0x0A }, "ATtiny2313A", 2 * kb,   0 },
  { { 0x1E, 0x92, 0x0D }, "ATtiny4313",  4 * kb,   0 },

  // ATtiny13 family
  { { 0x1E, 0x90, 0x07 }, "ATtiny13A",     1 * kb, 0 },

  // Atmega8A family
  { { 0x1E, 0x93, 0x07 }, "ATmega8A",     8 * kb, 256,
#if USE_ATMEGA8
    bootloader_hex,
#else
    0,
#endif
    Saddr,      // start address
#if USE_ATMEGA8
    sizeof bootloader_hex,
#else
    0,
#endif
    64,           // page size in bytes (for committing)
    Lfuse,         // fuse low byte: external clock, max start-up time
    Hfuse,         // fuse high byte: SPI enable, boot into bootloader, 1024 byte bootloader
    Efuse,         // fuse extended byte: brown-out detection at 2.7V
    Lockbit,
    testcode_hex,
    CODEaddr,
    sizeof testcode_hex,
    true
  },       // need to do timed writes, not polled ones
};  // end of signatures

// if signature found in above table, this is its index
int foundSig = -1;
byte lastAddressMSB = 0;

// execute one programming instruction ... b1 is command, b2, b3, b4 are arguments
//  processor may return a result on the 4th transfer, this is returned.
byte program (const byte b1, const byte b2 = 0, const byte b3 = 0, const byte b4 = 0)
{
  SPI.transfer (b1);
  SPI.transfer (b2);
  SPI.transfer (b3);
  return SPI.transfer (b4);
} // end of program

// read a byte from flash memory
byte readFlash (unsigned long addr)
{
  byte high = (addr & 1) ? 0x08 : 0;  // set if high byte wanted
  addr >>= 1;  // turn into word address

  // set the extended (most significant) address byte if necessary
  byte MSB = (addr >> 16) & 0xFF;
  if (MSB != lastAddressMSB)
  {
    program (loadExtendedAddressByte, 0, MSB);
    lastAddressMSB = MSB;
  }  // end if different MSB

  return program (readProgramMemory | high, highByte (addr), lowByte (addr));
} // end of readFlash

// write a byte to the flash memory buffer (ready for committing)
byte writeFlash (unsigned long addr, const byte data)
{
  byte high = (addr & 1) ? 0x08 : 0;  // set if high byte wanted
  addr >>= 1;  // turn into word address
  program (loadProgramMemory | high, 0, lowByte (addr), data);
} // end of writeFlash

// show a byte in hex with leading zero and optional newline
void showHex (const byte b, const boolean newline = false, const boolean show0x = true)
{
  if (show0x)
    Serial.print (F("0x"));
  // try to avoid using sprintf
  char buf [4] = { ((b >> 4) & 0x0F) | '0', (b & 0x0F) | '0', ' ' , 0 };
  if (buf [0] > '9')
    buf [0] += 7;
  if (buf [1] > '9')
    buf [1] += 7;
  Serial.print (buf);
  if (newline)
    Serial.println ();
}  // end of showHex

// convert a boolean to Yes/No
void showYesNo (const boolean b, const boolean newline = false)
{
  if (b)
    Serial.print (F("Yes"));
  else
    Serial.print (F("No"));
  if (newline)
    Serial.println ();
}  // end of showYesNo

// poll the target device until it is ready to be programmed
void pollUntilReady ()
{
  if (signatures [foundSig].timedWrites)
    delay (10);  // at least 2 x WD_FLASH which is 4.5 mS
  else
  {
    while ((program (pollReady) & 1) == 1)
    {}  // wait till ready
  }  // end of if
}  // end of pollUntilReady

// commit page
void commitPage (unsigned long addr)
{
  Serial.print (F("Committing page starting at 0x"));
  Serial.println (addr, HEX);

  addr >>= 1;  // turn into word address

  // set the extended (most significant) address byte if necessary
  byte MSB = (addr >> 16) & 0xFF;
  if (MSB != lastAddressMSB)
  {
    program (loadExtendedAddressByte, 0, MSB);
    lastAddressMSB = MSB;
  }  // end if different MSB

  program (writeProgramMemory, highByte (addr), lowByte (addr));
  pollUntilReady ();
}  // end of commitPage

// write specified value to specified fuse/lock byte
void writeFuse (const byte newValue, const byte instruction)
{
  if (newValue == 0)
    return;  // ignore

  program (progamEnable, instruction, 0, newValue);
  pollUntilReady ();
}  // end of writeFuse

void getFuseBytes ()
{
  Serial.print (F("LFuse = "));
  showHex (program (readLowFuseByte, readLowFuseByteArg2), true);
  Serial.print (F("HFuse = "));
  showHex (program (readHighFuseByte, readHighFuseByteArg2), true);
  Serial.print (F("EFuse = "));
  showHex (program (readExtendedFuseByte, readExtendedFuseByteArg2), true);
  Serial.print (F("Lock byte = "));
  showHex (program (readLockByte, readLockByteArg2), true);
  if (program (readLowFuseByte, readLowFuseByteArg2) != Lfuse || program (readHighFuseByte, readHighFuseByteArg2) != Hfuse || program (readExtendedFuseByte, readExtendedFuseByteArg2) != Efuse )
  {
    Serial.println("fuse test fail");
    error |= 1;
  }
  Serial.print (F("Clock calibration = "));
  showHex (program (readCalibrationByte), true);
}  // end of getFuseBytes

// burn the bootloader to the target device
void writeBootloader ()
{

  if (signatures [foundSig].bootloader == 0)
  {
    Serial.println (F("No bootloader support for this device."));
    return;
  }  // end if

  int i;

  byte lFuse = program (readLowFuseByte, readLowFuseByteArg2);

  byte newlFuse = signatures [foundSig].lowFuse;
  byte newhFuse = signatures [foundSig].highFuse;
  byte newextFuse = signatures [foundSig].extFuse;
  byte newlockByte = signatures [foundSig].lockByte;


  unsigned long addr = signatures [foundSig].loaderStart;
  unsigned int  len = signatures [foundSig].loaderLength;
  unsigned long pagesize = signatures [foundSig].pageSize;
  unsigned long pagemask = ~(pagesize - 1);
  const byte * bootloader = signatures [foundSig].bootloader;


  Serial.print (F("Bootloader address = 0x"));
  Serial.println (addr, HEX);
  Serial.print (F("Bootloader length = "));
  Serial.print (len);
  Serial.println (F(" bytes."));

  byte subcommand = 'U';
  unsigned long oldPage = addr & pagemask;
  // Serial.println (F("Type 'Q' to quit, 'V' to verify, or 'G' to program the chip with the bootloader ..."));
  char command = 'G';
  //  do
  //  {
  //    command = toupper (Serial.read ());
  //  } while (command != 'G' && command != 'V' && command != 'Q');
  //
  //  // let them do nothing
  //  if (command == 'Q')
  //    return;

  if (command == 'G')
  {

    // Automatically fix up fuse to run faster, then write to device
    if (lFuse != newlFuse)
    {
      if ((lFuse & 0x80) == 0)
        Serial.println (F("Clearing 'Divide clock by 8' fuse bit."));

      Serial.println (F("Fixing low fuse setting ..."));
      writeFuse (newlFuse, writeLowFuseByte);
      delay (1000);
      digitalWrite (RESET, HIGH); // latch fuse
      if (!startProgramming ())
        return;
      delay (1000);
    }

    Serial.println (F("Erasing chip ..."));
    program (progamEnable, chipErase);   // erase it
    delay (20);  // for Atmega8
    pollUntilReady ();
    Serial.println (F("Writing bootloader ..."));
    for (i = 0; i < len; i += 2)
    {
      unsigned long thisPage = (addr + i) & pagemask;
      // page changed? commit old one
      if (thisPage != oldPage)
      {
        commitPage (oldPage);
        oldPage = thisPage;
      }
      writeFlash (addr + i, pgm_read_byte(bootloader + i));
      writeFlash (addr + i + 1, pgm_read_byte(bootloader + i + 1));
    }  // end while doing each word

    // commit final page
    commitPage (oldPage);
    Serial.println (F("Written."));
  }  // end if programming

  Serial.println (F("Verifying ..."));

  // count errors
  unsigned int errors = 0;
  // check each byte
  for (i = 0; i < len; i++)
  {
    byte found = readFlash (addr + i);
    byte expected = pgm_read_byte(bootloader + i);
    if (found != expected)
    {
      if (errors <= 100)
      {
        Serial.print (F("Verification error at address "));
        Serial.print (addr + i, HEX);
        Serial.print (F(". Got: "));
        showHex (found);
        Serial.print (F(" Expected: "));
        showHex (expected, true);
      }  // end of haven't shown 100 errors yet
      errors++;
    }  // end if error
  }  // end of for

  if (errors == 0)
    Serial.println (F("No errors found."));
  else
  {
    error |= errors;
    Serial.print (errors, DEC);
    Serial.println (F(" verification error(s)."));
    if (errors > 100)
      Serial.println (F("First 100 shown."));
    return;  // don't change fuses if errors
  }  // end if

  if (command == 'G')
  {
    Serial.println (F("Writing fuses ..."));

    writeFuse (newlFuse, writeLowFuseByte);
    writeFuse (newhFuse, writeHighFuseByte);
    writeFuse (newextFuse, writeExtendedFuseByte);
    writeFuse (newlockByte, writeLockByte);

    // confirm them
    getFuseBytes ();
  }  // end if programming

  Serial.println (F("Done."));

} // end of writeBootloader


bool writecode ()
{
  int i;
  byte lFuse = program (readLowFuseByte, readLowFuseByteArg2);
  byte newlFuse = signatures [foundSig].lowFuse;
  unsigned long addr = signatures [foundSig].codeStart;
  unsigned int  len = signatures [foundSig].codeLength;
  unsigned long pagesize = signatures [foundSig].pageSize;
  unsigned long pagemask = ~(pagesize - 1);
  const byte * code = signatures [foundSig].code;
  Serial.print (F("code address = 0x"));
  Serial.println (addr, HEX);
  Serial.print (F("code length = "));
  Serial.print (len);
  Serial.println (F(" bytes."));

  byte subcommand = 'U';
  unsigned long oldPage = addr & pagemask;
  char command = 'G';
  if (command == 'G')
  {

    // Automatically fix up fuse to run faster, then write to device
    if (lFuse != newlFuse)
    {
      if ((lFuse & 0x80) == 0)
        Serial.println (F("Clearing 'Divide clock by 8' fuse bit."));

      Serial.println (F("Fixing low fuse setting ..."));
      writeFuse (newlFuse, writeLowFuseByte);
      delay (1000);
      digitalWrite (RESET, HIGH); // latch fuse
      if (!startProgramming())
        return  false;   //                            2015.05.07
      delay (1000);
    }


    pollUntilReady ();
    Serial.println (F("Writing code ..."));
    for (i = 0; i < len; i += 2)
    {
      unsigned long thisPage = (addr + i) & pagemask;
      // page changed? commit old one
      if (thisPage != oldPage)
      {
        commitPage (oldPage);
        oldPage = thisPage;
      }
      writeFlash (addr + i, pgm_read_byte(code + i));
      writeFlash (addr + i + 1, pgm_read_byte(code + i + 1));

    }  // end while doing each word

    // commit final page
    commitPage (oldPage);
    Serial.println (F("Written."));
  }  // end if programming

  Serial.println (F("Verifying ..."));

  // count errors
  unsigned int errors = 0;
  // check each byte
  for (i = 0; i < len; i++)
  {
    byte found = readFlash (addr + i);
    byte expected = pgm_read_byte(code + i);
    if (found != expected)
    {
      if (errors <= 100)
      {
        Serial.print (F("Verification error at address "));
        Serial.print (addr + i, HEX);
        Serial.print (F(". Got: "));
        showHex (found);
        Serial.print (F(" Expected: "));
        showHex (expected, true);
      }  // end of haven't shown 100 errors yet
      errors++;
    }  // end if error
  }  // end of for

  if (errors == 0)
  {
    Serial.println (F("No errors found."));

  }
  else
  {
    error = 1;
    Serial.print (errors, DEC);
    Serial.println (F(" verification error(s)."));
    if (errors > 100)
      Serial.println (F("First 100 shown."));
    return  false;  // don't change fuses if errors                 2015.05.07
  }  // end if
  Serial.println (F("Done."));
  return  true;                      //2015.05.07
}



bool startProgramming ()
{
  Serial.print (F("Attempting to enter programming mode ..."));
  digitalWrite (RESET, HIGH);  // ensure SS stays high for now
  SPI.begin ();
  SPI.setClockDivider (SPI_CLOCK_DIV64);

  byte confirm;
  pinMode (RESET, OUTPUT);
  pinMode (SCK, OUTPUT);
  unsigned int timeout = 0;

  // we are in sync if we get back programAcknowledge on the third byte
  do
  {
    delay (100);
    // ensure SCK low
    digitalWrite (SCK, LOW);
    // then pulse reset, see page 309 of datasheet
    digitalWrite (RESET, HIGH);
    delay (1);  // pulse for at least 2 clock cycles
    digitalWrite (RESET, LOW);
    delay (25);  // wait at least 20 mS
    SPI.transfer (progamEnable);
    SPI.transfer (programAcknowledge);
    confirm = SPI.transfer (0);
    SPI.transfer (0);

    if (confirm != programAcknowledge)
    {
      Serial.print (F("."));
      if (timeout++ >= ENTER_PROGRAMMING_ATTEMPTS)
      {
        Serial.println ();
        Serial.println (F("Failed to enter programming mode. Double-check wiring!"));
        return false;
      }  // end of too many attempts
    }  // end of not entered programming mode
  } while (confirm != programAcknowledge);

  Serial.println ();
  Serial.println (F("Entered programming mode OK."));
  return true;
}  // end of startProgramming

void stopProgramming ()
{
  SPI.end ();

  // turn off pull-ups, if any
  digitalWrite (RESET, LOW);
  digitalWrite (SCK,   LOW);
  digitalWrite (MOSI,  LOW);
  digitalWrite (MISO,  LOW);

  // set everything back to inputs
  pinMode (RESET, INPUT);
  pinMode (SCK,   INPUT);
  pinMode (MOSI,  INPUT);
  pinMode (MISO,  INPUT);

  Serial.println (F("Programming mode off."));
} // end of stopProgramming

void getSignature ()
{
  foundSig = -1;
  lastAddressMSB = 0;

  byte sig [3];
  Serial.print (F("Signature = "));
  for (byte i = 0; i < 3; i++)
  {
    sig [i] = program (readSignatureByte, 0, i);
    showHex (sig [i]);
  }  // end for each signature byte
  Serial.println ();

  for (int j = 0; j < NUMITEMS (signatures); j++)
  {
    if (memcmp (sig, signatures [j].sig, sizeof sig) == 0)
    {
      foundSig = j;
      Serial.print (F("Processor = "));
      Serial.println (signatures [j].desc);
      Serial.print (F("Flash memory size = "));
      Serial.print (signatures [j].flashSize, DEC);
      Serial.println (F(" bytes."));
      if (signatures [foundSig].timedWrites)
        Serial.println (F("Writes are timed, not polled."));
      return;
    }  // end of signature found
  }  // end of for each signature

  Serial.println (F("Unrecogized signature."));
}  // end of getSignature

void setup ()
{
  pinMode(blue, OUTPUT);
  digitalWrite(blue, LOW);
  program_init();
  while(!Serial.available());
  //  tools.wait_pushdown();
  // test(0);
}

void loop ()
{
  boot();
}





void program_init()
{
  Serial.begin (BAUD_RATE);
  while (!Serial) ;  // for Leonardo, Micro etc.
  Serial.println ();
  Serial.println (F("Atmega chip programmer."));
  Serial.println (F("Written by Nick Gammon."));
  Serial.println (F("Version " VERSION));
  Serial.println (F("Compiled on " __DATE__ " at " __TIME__));
  digitalWrite (RESET, HIGH);  // ensure SS stays high for now
  SPI.begin ();
  // slow down SPI for benefit of slower processors like the Attiny
  SPI.setClockDivider (SPI_CLOCK_DIV64);
  pinMode (CLOCKOUT, OUTPUT);
  // set up Timer 1
  //  TCCR1A = bit (COM1A0);  // toggle OC1A on Compare Match
  //  TCCR1B = bit (WGM12) | bit (CS10);   // CTC, no prescaling
  //  OCR1A =  0;       // output every cycle
}

int  boot()
{
  error = 1;
  int led = 1;
  unsigned long times = 0;
  unsigned long keep = 0;
  Serial.println("PROGRAM ANGIN");
  pinMode(blue, OUTPUT);
  digitalWrite(blue, LOW);

  if (startProgramming ())
  {
    error = 0;
    for (int n = 0; n < 2; n++)
    {
      getSignature ();
      getFuseBytes ();
      if (foundSig != -1)
      {
        writeBootloader ();
        delay(1000);
        //writecode();
      }
    }
  }

  while (startProgramming ())
  {
    Serial.println("PROGRAM OVER");
    delay(1000);
  }
}
