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

#ifndef _boottab_h_
#define _boottab_h_

#include "bootloader.h"

// Bootloader information table on unicorn

typedef struct {
    uint32_t version;					// version of boot loader
    __attribute__((noreturn))
	void (*panic) (uint32_t reason, uint32_t addr);	// bootloader panic function 
    uint32_t (*update) (void* ptr, hash32* hash);	// function to set firmware update pointer
    uint32_t (*crc32) (void* buf, uint32_t nwords);     // calculate CRC32
    void* svc;                                          // supervisor call
} boot_boottab;


// Supervisor call IDs

enum {
    BOOT_SVC_PANIC,             // panic: p1=type, p2=reason, p3=addr

    BOOT_SVC_FWBASE = 0x80      // all supervisor call ids below are reserved for bootloader
};

#endif
