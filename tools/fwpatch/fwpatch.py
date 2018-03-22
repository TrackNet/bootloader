#     _____                _        __     _
#    /__   \_ __ __ _  ___| | __ /\ \ \___| |_
#      / /\/ '__/ _` |/ __| |/ //  \/ / _ \ __|
#     / /  | | | (_| | (__|   '/ /\  /  __/ |_
#     \_\  |_|  \__,_|\___|_|\_\_\ \_\\___|\__|
#
# Copyright (c) 2016-2018 Trackio International AG
# All rights reserved.
#
# This file is subject to the terms and conditions
# defined in file 'LICENSE', which is part of this
# source code package.

import argparse
import struct
import sys

from intelhex import IntelHex
from binascii import crc32

parser = argparse.ArgumentParser(description='Patch a firmware hexfile with CRC and length.')
parser.add_argument('HEXFILE', type=str, help='the hexfile to process')
parser.add_argument('--check-only', action='store_true', help='Do not patch CRC and length')
args = parser.parse_args()

print('Processing %s' % args.HEXFILE)

ih = IntelHex()
ih.loadhex(args.HEXFILE)

start = ih.minaddr()
end = ih.maxaddr() + 1
sze = end - start
crc = crc32(ih.gets(start + 8, sze - 8))

if not args.check_only:
    ih.puts(start, struct.pack('<II', crc, sze))
    ih.write_hex_file(args.HEXFILE)

(hcrc, hsze) = struct.unpack('<II', ih.gets(start, 8))

print('CRC:  0x%08x (%s)' % (hcrc, 'ok' if hcrc == crc else 'invalid'))
print('Size: 0x%08x (%s)' % (hsze, 'ok' if hsze == sze else 'invalid'))

sys.exit(0 if hcrc == crc and hsze == sze else 1)
