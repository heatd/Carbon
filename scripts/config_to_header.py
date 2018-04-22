#!/usr/bin/env python3

import sys

with open("kernel.config") as file:
	with open(sys.argv[1], "w") as header:
		# TODO: Print copyright header?
		header.write("#ifndef _CARBON_CONFIG_H\n#define _CARBON_CONFIG_H\n\n")
		for line in file:
			line = line.replace('=', ' ')
			header.write('#define ' + line)
		header.write("\n#endif\n")
