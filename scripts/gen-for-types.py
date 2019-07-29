#!/bin/python3
integral_types = ["bool", "char", "short", "int", "long", "long long"]
special_modifiers = ["unsigned", "const"]
types_that_cant_be_unsigned = ["bool", "float", "double"]

def print_type_with_modifier(final_type):
	print("template<>\nstruct is_integral<%s> { constexpr static bool value = true; };\n" % final_type)

def can_be_unsigned(type):
	for t in types_that_cant_be_unsigned:
		if type == t:
			return False
	return True

def print_type(type):
	# so, basically, we need to append modifiers as modif + " "(space) + type name
	# Note - unsigned doesn't apply to certain types as bool, float, double, etc 
	# (although the 2 latter ones don't apply in this case)
	# first one - unsigned
	print_unsigned = can_be_unsigned(type)

	# Let's print them in the order of regular, unsigned, const,
	# const unsigned, because it's nice
	print_type_with_modifier(type)
	if print_unsigned == True:
		print_type_with_modifier(special_modifiers[0] + " " + type)
	# second one - const
	print_type_with_modifier(special_modifiers[1] + " " + type)
	# third one - const unsigned
	if print_unsigned == True:
		print_type_with_modifier(special_modifiers[1] + " " + special_modifiers[0] + " " + type)

for type in integral_types:
	print_type(type)	
