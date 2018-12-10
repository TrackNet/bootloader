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

#include "bootloader_impl.h"
#include "update.h"
#include "bootloader_hw.h"


// ------------------------------------------------
// CRC-32

static uint32_t boot_crc32 (void* buf, uint32_t nwords) {
    uint32_t* src = buf;
    uint32_t v;

    // enable crc peripheral
    RCC->AHBENR |= RCC_AHBENR_CRCEN;
    // reset crc peripheral, reverse bits on input and output (L0 only)
    CRC->CR = 0
#if defined(STM32L0)
	| CRC_CR_REV_IN | CRC_CR_REV_OUT
#endif
	| CRC_CR_RESET;

    while (nwords-- > 0) {
	v = *src++;
#if defined(STM32L1)
        v = __rbit(v);
#endif
	CRC->DR = v;
    }
    v = CRC->DR;

    // disable crc peripheral
    RCC->AHBENR &= ~RCC_AHBENR_CRCEN;

#if defined(STM32L1)
    v = __rbit(v);
#endif

    return ~v;
}


// ------------------------------------------------
// Panic handler

#if defined(BOOT_LED_GPIO)

extern void delay (int); // provided by util.S

#define _LED_INIT(p,n)	do { \
    (p)->MODER   = ((p)->MODER   & ~(3 << (2 * (n)))) | (1 << (2 * (n))); /* output          */ \
    (p)->PUPDR   = ((p)->PUPDR   & ~(3 << (2 * (n)))) | (0 << (2 * (n))); /* no pull-up/down */ \
    (p)->OTYPER  = ((p)->OTYPER  & ~(1 << (1 * (n)))) | (0 << (1 * (n))); /* push-pull       */ \
    (p)->OSPEEDR = ((p)->OSPEEDR & ~(3 << (2 * (n)))) | (0 << (2 * (n))); /* low speed       */ \
} while (0)
#define LED_INIT()	do { \
    GPIO_ENABLE(PORTN(BOOT_LED_GPIO)); \
    _LED_INIT(PORT(BOOT_LED_GPIO), PIN(BOOT_LED_GPIO)); \
} while (0)
#define LED_ON()	do { SET_PIN(BOOT_LED_GPIO, (BOOT_LED_GPIO & GPIO_F_ACTLOW) ? 0 : 1); } while (0)
#define LED_OFF()	do { SET_PIN(BOOT_LED_GPIO, (BOOT_LED_GPIO & GPIO_F_ACTLOW) ? 1 : 0); } while (0)

static void blink_value (uint32_t v) {
    // blink nibble-by-nibble
    // least-significant-nibble first, 0x0 -> 1 blink, 0xf -> 16 blinks
    do {
	uint32_t n = v & 0xf;
	do {
	    LED_ON();
	    delay(1);
	    LED_OFF();
	    delay(1);
	} while (n--);
	v >>= 4;
	delay(2);
    } while (v);
}
#endif

// force inlining of reset call
__attribute__((always_inline)) static void NVIC_SystemReset (void);

__attribute__((noreturn))
void boot_panic (uint32_t type, uint32_t reason, uint32_t addr) {
    // disable all interrupts
    __disable_irq();
    // startup MSI @2.1MHz
    RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_MSIRANGE) | RCC_ICSCR_MSIRANGE_5;
    RCC->CR |= RCC_CR_MSION;
    while ((RCC->CR & RCC_CR_MSIRDY) == 0);
    // switch clock source to MSI
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_MSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI);
    // no flash wait states
    FLASH->ACR &= ~FLASH_ACR_LATENCY;

#if defined(BOOT_LED_GPIO)
    LED_INIT();

    int repeat = 5;
    while (repeat-- > 0) {
	// blink long
	LED_ON();
	delay(5);
	LED_OFF();
	delay(5);
	// blink type
	blink_value(type);
	delay(5);
	// blink reason
	blink_value(reason);
	delay(5);
	// blink address
	blink_value(addr);
	delay(5);
    }
#endif
    NVIC_SystemReset();
    // not reached
    while (1);
}

__attribute__((noreturn, naked))
static void fw_panic (uint32_t reason, uint32_t addr) {
    boot_panic(BOOT_PANIC_TYPE_FIRMWARE, reason, addr);
}


// ------------------------------------------------
// Flash functions

typedef void (*wr_fl_hp) (uint32_t*, uint32_t*);

typedef struct {
    boot_uphdr* fwup;
    wr_fl_hp wf_func;
} up_ctx;

static void unlock_flash (void) {
    // unlock flash registers
    FLASH->PEKEYR = 0x89ABCDEF; // FLASH_PEKEY1
    FLASH->PEKEYR = 0x02030405; // FLASH_PEKEY2
    // enable flash programming
    FLASH->PRGKEYR = 0x8C9DAEBF; // FLASH_PRGKEY1;
    FLASH->PRGKEYR = 0x13141516; // FLASH_PRGKEY2;
    // enable flash erase and half-page programming
    FLASH->PECR |= FLASH_PECR_PROG;
}

static void relock_flash (void) {
    FLASH->PECR |= FLASH_PECR_PELOCK;
}

static void check_eop (uint32_t panic_addr) {
    if (FLASH->SR & FLASH_SR_EOP) {
	FLASH->SR = FLASH_SR_EOP;
    } else {
	boot_panic(BOOT_PANIC_TYPE_BOOTLOADER,
		BOOT_PANIC_REASON_FLASH, panic_addr);
    }
}

extern uint32_t wr_fl_hp_begin;	// provided by util.S
extern uint32_t wr_fl_hp_end;	// provided by util.S

#define WR_FL_HP_WORDS	(&wr_fl_hp_end - &wr_fl_hp_begin)

static wr_fl_hp prep_wr_fl_hp (uint32_t* funcbuf) {
    for (int i = 0; i < WR_FL_HP_WORDS; i++) {
	funcbuf[i] = (&wr_fl_hp_begin)[i];
    }
    return THUMB_FUNC(funcbuf);
}

#if 0
static void write_flash (uint32_t* dst, uint32_t* src, uint32_t nwords, bool erase) {
    uint32_t funcbuf[WR_FL_HP_WORDS];
    wr_fl_hp wf_func = prep_wr_fl_hp(funcbuf);

    unlock_flash_prog();
    while (nwords > 0) {
	if (erase && (((uintptr_t) dst) & 127) == 0) {
	    // erase page
	    FLASH->PECR |= FLASH_PECR_ERASE;
	    *dst = 0;
	    while (FLASH->SR & FLASH_SR_BSY);
	    check_eop(2);
	    FLASH->PECR &= ~FLASH_PECR_ERASE;
	}
	if ((((uintptr_t) dst) & 63) == 0 && nwords >= 16) {
	    // write half page
	    FLASH->PECR |= FLASH_PECR_FPRG;
	    wf_func(dst, src);
	    check_eop(3);
	    FLASH->PECR &= ~FLASH_PECR_FPRG;
	    src += 16;
	    dst += 16;
	    nwords -= 16;
	} else {
	    // write word
	    *dst++ = *src++;
	    while (FLASH->SR & FLASH_SR_BSY);
	    check_eop(4);
	    nwords -= 1;
	}
    }
    relock_flash();
}
#endif

static void ee_write (uint32_t* dst, uint32_t val) {
    *dst = val;
    while (FLASH->SR & FLASH_SR_BSY);
}


// ------------------------------------------------
// Update glue functions

uint32_t up_install_init (void* ctx, uint32_t size, void** pdst) {
    up_ctx* uc = ctx;
    if (ROUND_PAGE_SZ(size) > ((uintptr_t) uc->fwup - BOOT_FW_BASE)) {
	// new firmware would overwrite update
	return BOOT_E_SIZE;
    }
    *pdst = (void*) BOOT_FW_BASE;
    return BOOT_OK;
}

void up_flash_wr_page (void* ctx, void* dst, void* src) {
    up_ctx* uc = ctx;
    // erase if on page boundary
    if ((((uintptr_t) dst) & (FLASH_PAGE_SZ - 1)) == 0) {
	// erase page
	FLASH->PECR |= FLASH_PECR_ERASE;
	*((uint32_t*) dst) = 0;
	while (FLASH->SR & FLASH_SR_BSY);
	check_eop(0);
	FLASH->PECR &= ~FLASH_PECR_ERASE;
    }
    // write half page
    FLASH->PECR |= FLASH_PECR_FPRG;
    uc->wf_func(dst, src);
    check_eop(1);
    FLASH->PECR &= ~FLASH_PECR_FPRG;
}

void up_flash_unlock (void* ctx) {
    unlock_flash();
}

void up_flash_lock (void* ctx) {
    relock_flash();
}


// ------------------------------------------------
// Update functions

static void do_install (boot_uphdr* fwup) {
    uint32_t funcbuf[WR_FL_HP_WORDS];
    up_ctx uc = {
	.wf_func = prep_wr_fl_hp(funcbuf),
	.fwup = fwup,
    };
    update(&uc, fwup, true);
}

static uint32_t set_update (void* ptr, hash32* hash) {
    uint32_t rv;
    if (ptr == NULL) {
	rv = BOOT_OK;
    } else {
	rv = update(NULL, ptr, false);
    }
    if (rv == BOOT_OK) {
	boot_config* cfg = (boot_config*) BOOT_CONFIG_BASE;
	// unlock EEPROM
	FLASH->PEKEYR = 0x89ABCDEF; // FLASH_PEKEY1
	FLASH->PEKEYR = 0x02030405; // FLASH_PEKEY2
	// copy hash
	if (hash) {
	    for (int i = 0; i < 8; i++) {
		ee_write(&cfg->hash.w[i], hash->w[i]);
	    }
	}
	// set update pointer
	ee_write(&cfg->fwupdate1, (uint32_t) ptr);
	ee_write(&cfg->fwupdate2, (uint32_t) ptr);
	// relock EEPROM
	FLASH->PECR |= FLASH_PECR_PELOCK;
    }
    return rv;
}


// ------------------------------------------------
// Bootloader main entry point

void* bootloader (void) {
    boot_fwhdr* fwh = (boot_fwhdr*) BOOT_FW_BASE;
    boot_config* cfg = (boot_config*) BOOT_CONFIG_BASE;

    uint32_t flash_sz = FLASH_SZ();

    if (cfg->fwupdate1 == cfg->fwupdate2) {
	boot_uphdr* fwup = (boot_uphdr*) cfg->fwupdate1;
	if (fwup != NULL
		&& ((intptr_t) fwup & 3) == 0
		&& (intptr_t) fwup >= FLASH_BASE
		&& sizeof(boot_uphdr) <= flash_sz - ((intptr_t) fwup - FLASH_BASE)
		&& fwup->size >= sizeof(boot_uphdr)
		&& (fwup->size & 3) == 0
		&& fwup->size <= flash_sz - ((intptr_t) fwup - FLASH_BASE)
		&& boot_crc32(((unsigned char*) fwup) + 8, (fwup->size - 8) >> 2) == fwup->crc
		&& true /* TODO hardware id match */ ) {
	    do_install(fwup);
	}
    }

    // verify integrity of current firmware
    if (fwh->size < sizeof(boot_fwhdr)
	    || fwh->size > (flash_sz - (BOOT_FW_BASE - FLASH_BASE))
	    || boot_crc32(((unsigned char*) fwh) + 8, (fwh->size - 8) >> 2) != fwh->crc) {
	boot_panic(BOOT_PANIC_TYPE_BOOTLOADER, BOOT_PANIC_REASON_CRC, 0);
    }

    // clear fwup pointer in EEPROM if set
    if (cfg->fwupdate1 != 0 || cfg->fwupdate2 != 0) {
	set_update(NULL, NULL);
    }

    // return entry point
    return (void*) fwh->entrypoint;
}


// ------------------------------------------------
// Bootloader information table

__attribute__((section(".boot.boottab"))) const boot_boottab boottab = {
    .version	= 0x100,
    .update	= set_update,
    .panic	= fw_panic,
    .crc32      = boot_crc32,
};
