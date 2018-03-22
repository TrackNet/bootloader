/*     _____                _        __     _
 *    /__   \_ __ __ _  ___| | __ /\ \ \___| |_
 *      / /\/ '__/ _` |/ __| |/ //  \/ / _ \ __|
 *     / /  | | | (_| | (__|   '/ /\  /  __/ |_
 *     \_\  |_|  \__,_|\___|_|\_\_\ \_\\___|\__|
 *
 * Copyright (c) 2016-2018 Trackio International AG
 * All rights reserved.
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE', which is part of this
 * source code package.
 *
 */

#ifndef _bootloader_h_
#define _bootloader_h_


// Panic types
#define BOOT_PANIC_TYPE_EXCEPTION	0	// Exception handler
#define BOOT_PANIC_TYPE_BOOTLOADER	1	// Bootloader (reason codes see below)
#define BOOT_PANIC_TYPE_FIRMWARE	2	// Firmware (reason codes are application defined)


// Panic reason codes for type bootloader
#define BOOT_PANIC_REASON_FWRETURN	0	// firmware returned unexpectedly
#define BOOT_PANIC_REASON_CRC		1	// firmware CRC verification failed
#define BOOT_PANIC_REASON_FLASH		2	// error writing flash


#ifndef ASSEMBLY

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BOOT_CONFIG_BASE	DATA_EEPROM_BASE
#define BOOT_CONFIG_SZ		64


// Bootloader return values (don't change values!)
enum {
    BOOT_OK,
    BOOT_E_UNKNOWN,
    BOOT_E_SIZE,
};

// SHA-256 hash
typedef union {
    uint8_t b[32];
    uint32_t w[8];
} hash32;

// Bootloader information table
typedef struct {
    uint32_t version;					// version of boot loader (values below 256 are reserved for legacy bootloaders)
    uint32_t (*update) (void* ptr, hash32* hash);	// function to set firmware update pointer
    __attribute__((noreturn))
	void (*panic) (uint32_t reason, uint32_t addr);	// bootloader panic function 

    // TODO: extensions
} boot_boottab;


// Firmware header
typedef struct {
    uint32_t	crc;		// firmware CRC
    uint32_t	size;		// firmware size (in bytes, including this header)
    /* -- everything below until end (size-8) is included in CRC -- */
    uint32_t	entrypoint;	// address of entrypoint
} boot_fwhdr;


// Hardware identifier (EUI-48, native byte order)
typedef union {
    struct {
	uint32_t a;
	uint16_t b;
    };
    uint8_t bytes[6];
} eui48;

static inline uint64_t eui2int (eui48* eui) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((uint64_t) eui->b << 32) | eui->a;
#else
    return ((uint64_t) eui->a << 16) | eui->b;
#endif
}


// Update header
typedef struct {
    uint32_t	crc;		// update CRC
    uint32_t	size;		// update size (in bytes, including this header)
    /* -- everything below until end (size-8) is included in CRC -- */
    uint32_t	fwsize;		// firmware size (in bytes, including header)
    uint32_t	fwcrc;		// firmware CRC (once unpacked)
    eui48	hwid;		// hardware target
    uint8_t	uptype;		// update type
    uint8_t	rfu;		// RFU
} boot_uphdr;

#endif
#endif
