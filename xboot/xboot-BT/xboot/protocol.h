#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "xboot.h"

// General Commands
#define CMD_SYNC                '\x1b'

// Informational Commands
#define CMD_CHECK_AUTOINCREMENT 'a'
#define CMD_CHECK_BLOCK_SUPPORT 'b'
#define CMD_PROGRAMMER_TYPE     'p'
#define CMD_DEVICE_CODE         't'
#define CMD_PROGRAM_ID          'S'
#define CMD_VERSION             'V'
#define CMD_READ_SIGNATURE      's'

// Addressing
#define CMD_SET_ADDRESS         'A'
#define CMD_SET_EXT_ADDRESS     'H'

// Erase
#define CMD_CHIP_ERASE          'e'

// Block Access
#define CMD_BLOCK_LOAD          'B'
#define CMD_BLOCK_READ          'g'

// Byte Access
#define CMD_READ_BYTE           'R'
#define CMD_WRITE_LOW_BYTE      'c'
#define CMD_WRITE_HIGH_BYTE     'C'
#define CMD_WRITE_PAGE          'm'
#define CMD_WRITE_EEPROM_BYTE   'D'
#define CMD_READ_EEPROM_BYTE    'd'

// Lock and Fuse Bits
#define CMD_WRITE_LOCK_BITS     'l'
#define CMD_READ_LOCK_BITS      'r'
#define CMD_READ_LOW_FUSE_BITS  'F'
#define CMD_READ_HIGH_FUSE_BITS 'N'
#define CMD_READ_EXT_FUSE_BITS  'Q'

// Bootloader Commands
#define CMD_ENTER_PROG_MODE     'P'
#define CMD_LEAVE_PROG_MODE     'L'
#define CMD_EXIT_BOOTLOADER     'E'
#define CMD_SET_LED             'x'
#define CMD_CLEAR_LED           'y'
#define CMD_SET_TYPE            'T'

#define CMD_CRC                 'h'
#define CMD_CRC_WRITE			'z'

// I2C Address Autonegotiation Commands
#define CMD_AUTONEG_START       '@'
#define CMD_AUTONEG_DONE        '#'

// Memory types for block access
#define MEM_EEPROM              'E'
#define MEM_FLASH               'F'
#define MEM_USERSIG             'U'
#define MEM_PRODSIG             'P'

// Sections for CRC checks
#define SECTION_FLASH           'F'
#define SECTION_APPLICATION     'A'
#define SECTION_BOOT            'B'
#define SECTION_CUSTOM			'C'
#define SECTION_APP             'a'
#define SECTION_APP_TEMP        't'

// Command Responses
#define REPLY_ACK               '\r'
#define REPLY_YES               'Y'
#define REPLY_ERROR             '?'

#endif // __PROTOCOL_H
