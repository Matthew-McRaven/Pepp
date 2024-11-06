from types import SimpleNamespace

# Given an object/module, return the names that look like FORTH implementation functions
def extract(object):
	return [item:=getattr(object, key) for key in dir(object) if "FORTH" in dir(getattr(object, key))]

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

# Use as a decorator
# Acts like "defcode" macro from JoneForth
# Current useful args:
#   immediate:      Should the VM set the IMMEDIATE flag when defining the word?
#   pad:            Number of padding bytes that should follow the entry? Default: 0
#   refs:           Which words should be inserted into the <NAME>.FORTH.refs dict? Pass in a list, receive a dict in the function.
#   priority:       Relative ordering  of words. Defaults to 100.
def	NATIVE(name, **kwargs):
	def wrapper(function):
		function.FORTH = {"priority": 100, "native":True, "refs":[], **kwargs, "name": name}
		return function
	return wrapper
def ALIAS(old, new,  **kwargs):
	return SimpleNamespace(FORTH={
		"priority": 100,
		"alias": old,
		"name": new,
		"immediate":False,
		**kwargs
	})
def INTERPRET(name, definition, **kwargs):
	refs = set()
	for word in set(definition.split()):
		try: int(word)
		except: refs.add(word)

	return SimpleNamespace(FORTH={
		"priority": 100,
		"native": False,
		**kwargs,
		"refs": refs,
		"definition": definition,
		"name": name,
	})