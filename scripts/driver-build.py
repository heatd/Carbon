#!/usr/bin/env python3

# We start with an empty list and we start filling it up as we discover drivers
drivers = []
configured_drivers = []
configured_modules = []

import os
import string
import io
import multiprocessing
import sys


def get_driver_name(config_option):
	for driver in drivers:
		if driver[0] == config_option:
			return driver[1]
	return ''

def write_objs(file, driver_name, makefile):
	
	file.write(makefile.read())
	return

def output_drivers():
# Dump the driver makefiles as a Makefile-ish thingy
# TODO: Maybe dumping this as an already preprocessed Makefile would be a
# better design?
	with open('drivers.config', 'w') as file:
		for driver in configured_drivers:
			with open('drivers/' + driver + '/Makefile') as makefile:
				write_objs(file, driver, makefile)
		file.flush()

if len(sys.argv) == 1:
	print('driver-build.py: Usage: driver-build.py [build target]')
	exit

build_target = sys.argv[1]
make_target = ''

if build_target == 'build':
	make_target = 'all'
elif build_target == 'clean':
	make_target = 'clean'
else:
	raise(Exception('Unknown target ' + build_target))

for driver in os.listdir("drivers"):
	# Open the build config
	with open('drivers/' + driver + '/build-config', 'r') as build_config:
		config_option = build_config.readline()
		# Now that we have the name of the CONFIG_*, strip the trailing
		# newline, create a tuple with the config option and the driver
		# and append it
		config_option = config_option.strip('\n')
		drivers.append((config_option, driver))

with open('kernel.config') as config:
	for line in config:
		if line[0] == '#':	# Comment, skip
			continue
		# Every other line has a CONFIG_*=x
		# split by it.
		s = line.split('=')
		if len(s) == 0:
			raise(Exception('Invalid line ' + line))
		is_module = False

		# Check if the possible driver is to be built as a module
		if s[1] == 'm':
			is_module = True
		elif s[1] == 'n':
			continue
		driver_name = get_driver_name(s[0])

		# Not a driver
		if driver_name == '':
			continue
		# Add it to a list
		if is_module == True:
			configured_modules.append(driver_name)
		else:
			configured_drivers.append(driver_name)

# Don't forget to clean the drivers.config that we may have produced
if build_target == 'clean':
	os.remove('drivers.config')
elif build_target == 'build':
	output_drivers()
 
if len(configured_modules) != 0:
	print('Building modules')
else:
	exit

nr_cpus = multiprocessing.cpu_count()
for module in configured_modules:
	os.system('make -j' + str(nr_cpus) + ' -C drivers/' + module + ' ' + make_target)
