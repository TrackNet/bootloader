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

#ifndef _bootloader_impl_h_
#define _bootloader_impl_h_

#include "bootloader.h"
#include "boottab.h"

// Convert pointer to Thumb function (set least-significant bit)
#define THUMB_FUNC(p)	((void*) (((intptr_t) (p)) | 1))

extern uint32_t _ebl;
#define BOOT_FW_BASE	((uint32_t) (&_ebl))

#define BOOT_CONFIG_BASE	DATA_EEPROM_BASE	// XXX
#define BOOT_CONFIG_SZ		64			// XXX


// ------------------------------------------------
// Bootloader configuration

typedef struct {
    uint32_t	fwupdate1;	// 0x00 pointer to valid update
    uint32_t	fwupdate2;	// 0x04 pointer to valid update
    hash32	hash;		// 0x08 SHA-256 hash of valid update

    uint8_t	rfu[24];	// 0x28 RFU
} boot_config;

#endif
