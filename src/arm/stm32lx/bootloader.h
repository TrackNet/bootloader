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

// Bootloader information table
typedef struct {
    uint32_t version;					// version of boot loader (values below 256 are reserved for legacy bootloaders)
    uint32_t (*update) (void* ptr, uint8_t* hash);	// function to set firmware update pointer
    __attribute__((noreturn))
	void (*panic) (uint32_t reason, uint32_t addr);	// bootloader panic function 

    // TODO: extensions
} boot_boottab;


// Firmware header
typedef struct {
    uint32_t	crc;		// firmware CRC
    uint32_t	size;		// firmware size (in bytes, including this header)
    uint32_t	entrypoint;	// address of entrypoint
} boot_fwhdr;


// Hardware identifier
typedef struct {
    uint8_t oui[3];		// OUI
    uint8_t hwid[3];		// hardware identifier
} boot_hwid;

// Update header
typedef struct {
    uint32_t	crc;		// firmware CRC
    uint32_t	size;		// firmware size (in bytes, including this header)
} boot_uphdr;

#endif
#endif
