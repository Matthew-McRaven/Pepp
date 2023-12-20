import enum

from .strings import readStr as _readStr
from .utils import number_impl, as_hex as _as_hex


# The first data cell of each executable word MUST be "native" code
class Offsets(enum.IntEnum):
	LINK = 0
	CODELEN = 2
	STRLEN = 3
	STR = 4
	
# Bit masks for possible dictionary entry flags.
class Flags(enum.IntEnum):
	IMMEDIATE = 0x80
	HIDDEN = 0x20
	# LEN includes includes hidden bit.
	# This way, a hidden entry will NEVER match against a string, since
	# the reported length of the hidden word is longer than the max word size (31).
	# Trick borrowed from JoensForth.
	LEN = 0x3f 
	MAX_LEN = 0x1f # Actual max len is 0x1f
	FLAG_MASK = 0xE0

def visit(VM, functor, condition=lambda addr: True):
	current = VM.tcb.latest()
	while current != 0 and condition(current):
		functor(current)
		current = link(VM, current)

# Helper to walk FORTH dictionary, returning a pointer to the head
# of the first matching entry.
def find(VM, nameLen, name, matchHidden=False):
	def functor(addr):
		# Use cascading conditionals to avoid nested ones.
		# Each non-default conditional will prevent
		# Do not match against a hidden entry if we respect the flag
		# Mask out non-length bits of the strlen field.
		entryLen = VM.memory.read_b8(addr + Offsets.STRLEN, signed=False)
		if not matchHidden and (entryLen & Flags.HIDDEN): pass
		elif nameLen != entryLen & Flags.LEN: pass
		elif name != _readStr(VM, addr + Offsets.STR): pass
		else:
			functor.found = True
			functor.ret = addr

	functor.found, functor.ret = False, 0
	visit(VM, functor, lambda addr: functor.found == False)
	return functor.ret

# Native accessors for FORTH dictionary

def link(VM, address): return VM.memory.read_b16(address + Offsets.LINK, signed=False)
def name(VM, address): return _readStr(VM, address + Offsets.STR)
def namelen(VM, address): return VM.memory.read_b8(address + Offsets.STRLEN, signed=False) & Flags.LEN
def codelen(VM, address): return VM.memory.read_b8(address + Offsets.CODELEN, signed=False)
def flags(VM, address): return VM.memory.read_b8(address + Offsets.STRLEN, signed=False) & Flags.FLAG_MASK

# Assuming address points to the link field of a dictionary entry, return the address of the first code word.
def cwa(VM, address):
	# Mask out non-length bits of the strlen field.
	strlen = VM.memory.read_b8(address + Offsets.STRLEN, signed=False) & Flags.MAX_LEN
	# Add 1 and mask out low bit to get actual length of string
	# Mask out other flags
	# EVEN: 2 => 4 => 4, which is right because we pad evens with 2 nulls
	# ODD: 1 =>3 => 2, which is right because we pad odds with 1 null
	offset = ((strlen + 2) & 0xFE)
	return address + Offsets.STR + offset

# Given an address, attempt to parse it as a dictionary header.
def entry(VM, address):
	ret = {}
	ret["link"] = link(VM, address)
	ret["head"] = address
	ret["codelen"] = codelen(VM, address)
	ret["strlen"] = namelen(VM, address)
	ret["flags"] = flags(VM, address)
	ret["str"] = address + Offsets.STR
	ret["cwa"] = cwa(VM, address)
	return ret
	
# Pretty print the entire contents of the FORTH dictionary.
def dump(VM):

	def functor(addr):
		_link, _cwa = link(VM, addr), cwa(VM, addr)
		_strlen, _str = namelen(VM, addr), name(VM, addr)
		_flagBits = (flags(VM, addr) & Flags.FLAG_MASK)
		keys = ["I" if _flagBits & Flags.IMMEDIATE else ""] + ["H" if _flagBits & Flags.HIDDEN else ""]
		_flags = (3*" "+"".join(keys))[-3:]
		strs = []
		for i in range(codelen(VM, addr)//2): strs.append((4*"0" + hex(VM.memory.read_b16(_cwa+2*i,signed=False))[2:])[-4:])
		#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
		print(f"{_as_hex(_link)} <= {_as_hex(addr):4} {_flags:3} {_strlen:2}{_str:10} ({codelen(VM, addr):4})*[{hex(_cwa):4}]={' '.join(strs)}")

	visit(VM, functor)
	current, prev = VM.tcb.latest(), 0

# Find the most likely dictionary entry from a given address that is in the middle of an entry
def nearest_header(VM, target):
	def functor(addr):
		if  addr < target: functor.found, functor.ret = True, addr

	functor.found = False
	functor.ret = VM.tcb.latest()
	visit(VM, functor, lambda addr: functor.found == False)
	return functor.ret

# Walks the dictionary, finding the first entry which matches the name.
# Returns None if no matches.
def addr_from_name(VM, targetname):
	nuHeader = None

	def functor(addr):
		nonlocal nuHeader
		if targetname == name(VM, addr):
			nuHeader = entry(VM, addr)
			functor.found = True

	functor.found = False
	visit(VM, functor, lambda addr: functor.found == False)
	return nuHeader

		
# Helpers to create the initial native forth definitions
def header(VM, name, immediate=False, hidden=False):
	# u16 (link)
	VM.memory.write_b16(VM.herePP(2), VM.tcb.latest(), signed=False);
	VM.tcb.latest(VM.tcb.here() - 2)
	# u8 (number of token bytes)
	VM.memory.write_b8(VM.herePP(1), 0, signed=False)
	# u8 (flags | string length)
	VM.memory.write_b8(VM.herePP(1), 
		(Flags.IMMEDIATE if immediate else 0)
		| (Flags.HIDDEN if hidden else 0)
		| (len(name) & Flags.LEN), signed=False)
	# n * u8 name string; plus 1 or 2 u8 of null
	for letter in bytearray(name, "utf-8"): VM.memory.write_b8(VM.herePP(1), letter, signed=False)
	# Always null terminate strings, and place next words on a 16b boundary
	# So, pad evens with 2*null, odds with 1.
	if VM.tcb.here() % 2 == 0: VM.memory.write_b8(VM.herePP(1), 0, signed=False)
	VM.memory.write_b8(VM.herePP(1), 0, signed=False)
	# Helper to dump the bytes of the entry
	#print(binascii.hexlify(VM.memory[VM.latest:VM.here]).decode("utf-8"))
def writeTokens(VM, tokens):
	# Needed to return head of code field
	cwa = VM.tcb.here()
	# n*u16code list
	for token in tokens: VM.memory.write_b16(VM.herePP(2), token, signed=True if token<0 else False);
	# Update code length field
	VM.memory.write_b8(VM.tcb.latest()+2, 2*len(tokens), signed=False)
	#print(f"Defined {(10*' '+name)[-10:]}, from {(2*'0'+hex(old)[2:])[-2:]:2}..{(2*'0'+hex(VM.here)[2:])[-2:]:2}; strlen {len(name):2}, memlen is {VM.here-old:2}")
	return cwa
def defcode(VM, name, tokens, immediate=False):
	header(VM, name, immediate=immediate)
	return writeTokens(VM, tokens)

# Word is a sequence. [0] = name as str, [1] = flags, [2] = sequence of word names that are already in the dictionary.
def defforth(VM, word):
	enter = [VM.memory.read_b16(cwa(VM, find(VM, len("ENTER"), "ENTER")), False)]
	name, flags, tokenStrs = word
	entries = []
	for token in tokenStrs:
		num, isNum = number_impl(token, 10)
		if ( addr := find(VM, len(token), token)) > 0: entries.append(cwa(VM, addr))
		elif isNum: entries.append(num)
		else: raise Exception("That didn't work")
	#print(*zip(tokenStrs, [hex(x) for x in entries])) # Debug helper when entries contains 0.
	header(VM, name, True if (flags & Flags.IMMEDIATE) else False)
	writeTokens(VM, enter+entries)