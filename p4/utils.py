# Helper for formatting a 2 byte hex value from int		
def as_hex(value): return f"{(4*'0' + hex(value)[2:])[-4:]}"

# Get the number of bytes needed to hold an int.
# From: https://stackoverflow.com/a/14329943
def bytes(intVal):
	return (intVal.bitLength() + 7) // 8


def number_impl(text, base=10):
	number, flag = 0, 1
	try:
		number = int(text, base)
	except:
		flag = 0
	return number, flag

def IMMEDIATE(function):
	def wrapper(function):
		if not hasattr(function, "FORTH"): function.FORTH={}
		function.FORTH["immediate"] = True
		return function	
	return wrapper
	
# Use as a decorator
# Equivalent of "NEXT" macro in JonesForth which auto-advances instruction pointer after
# executing function body.	
def NEXT(function):
	return lambda VM: (function(VM), VM.next())

# Use as a decorator
# Used to assign a FORTH name and flags to a word
# Acts like "defcode" macro from JoneForth
def NAMED(name):
	def wrapper(function):
		if not hasattr(function, "FORTH"): function.FORTH={}
		function.FORTH["name"] = name
		return function	
	return wrapper

# Use as a decorator
# Used to ask the bootstrap function to inject the dict ptr of another word into this word.
# Needed since function implementations do not "live" inside FORTH virtual memory.
def REFERS(name):
	def wrapper(function):
		if not hasattr(function, "FORTH"): function.FORTH={}
		if "refs" not in function.FORTH: function.FORTH["refs"] = {}
		function.FORTH["refs"][name] = None
		return function
	return wrapper

# Use as a decorator
# Insert a number of bytes (of 0's) after defining this dictionary entry
# The dictionary MUST assign the attribute "pad" to the function, with pad's value being the address of the first pad byte
def PADDED(count):
	def wrapper(function):
		if not hasattr(function, "FORTH"): function.FORTH={}
		function.FORTH["pad"] = count
		return function	
	return wrapper
