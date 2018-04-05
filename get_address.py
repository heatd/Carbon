#!/bin/python3

import sys
import os

if len(sys.argv) == 2:
	base = 0xffffffff80000000
else:
	base = int(sys.argv[2], 16)
relocated_addr = int(sys.argv[1], 16) - base

os.system("addr2line -e kernel/carbon-unstripped " + format(relocated_addr, 'x'))
os.system("objdump -d kernel/carbon-unstripped | grep " + format(relocated_addr, 'x'))
