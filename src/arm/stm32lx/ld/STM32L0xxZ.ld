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

MEMORY {
    RAM(xrw)	: ORIGIN = 0x20000000, LENGTH = 20K
    BLFLASH(rx)	: ORIGIN = 0x08000000, LENGTH = 12K
    FWFLASH(rx)	: ORIGIN = 0x08000000 + LENGTH(BLFLASH), LENGTH = 192K - LENGTH(BLFLASH)
}
