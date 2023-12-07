import enum

from .strings import readStr as _readStr

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
	MAX_LEN = 0x1f# Actual max len is 0x1f

# Helper to walk FORTH dictionary, returning a pointer to the head
# of the first matching entry.
def find(VM, nameLen, name, matchHidden=False):
	# Keep track of the previous visited dict entry in "last"
	current, last = VM.tcb.latest, 0
	# Prevent infinite loop if we have an entry point to itself by checking that we aren't revisiting self 
	while	current != 0 and current != last:
		# Use cascading conditionals to avoid nested ones.
		# Each non-default conditional will prevent
		# Do not match against a hidden entry if we respect the flag
		# Mask out non-length bits of the strlen field.
		entryLen = VM.memory.read_b8(current + Offsets.STRLEN, signed=False)
		if not matchHidden and (entryLen & Flags.HIDDEN): pass 
		elif nameLen != entryLen & Flags.LEN: pass
		elif name != _readStr(VM, current+Offsets.STR): pass
		else: return current
		current, last = VM.memory.read_b16(current, signed=False), current
	return 0
		
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
	
# Native accessors for FORTH dictionary

# Given an address, attempt to parse it as a dictionary header.
def entry(VM, address):
	ret = {}
	ret["link"] = VM.memory.read_b16(address + Offsets.LINK, signed=False)
	ret["head"] = address
	ret["codelen"] = VM.memory.read_b8(address + Offsets.CODELEN, signed=False)
	ret["strlen"] = VM.memory.read_b8(address + Offsets.STRLEN, signed=False) & Flags.LEN
	ret["str"] = address + Offsets.STR
	ret["cwa"] = cwa(VM, address)
	return ret
	
# Pretty print the entire contents of the FORTH dictionary.
def dump(VM): 	
	current, prev = VM.tcb.latest, 0
	while	current != 0:
		header = entry(VM, current)
		cwa = header["cwa"]
		exec_token = VM.memory.read_b16(cwa, signed=True)	
		strs = []
		for i in range(header["codelen"]//2): strs.append((4*"0" + hex(VM.memory.read_b16(cwa+2*i,signed=False))[2:])[-4:])
		#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
		print(f"{hex(header['link']):4} <= {hex(header['str']-4):4} {header['strlen']:2}{_readStr(VM,header['str']):10} *[{hex(cwa):4}]={' '.join(strs)}")
		# Dump the dict entry in binary
		#s=binascii.hexlify(VM.memory[entry['str']-4:cwa + 2*entry["flag"]]).decode("utf-8")
		#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
		prev, current = current, header["link"]
				
# Walks the dictionary, finding the first entry which matches the name.
# Returns None if no matches.
def addr_from_name(VM, name):
	current = VM.tcb.latest
	while	current != 0:
		header = entry(VM, current)
		current = header["link"]
		entryName = _readStr(VM, header["str"])
		if name == entryName: return header
	return None
		
# Helpers to create the initial native forth definitions
def header(VM, name, immediate=False, hidden=False):
	# u16 (link)
	VM.memory.write_b16(VM.herePP(2), VM.tcb.latest, signed=False);
	VM.tcb.latest = VM.tcb.here - 2
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
	if VM.tcb.here % 2 == 0: VM.memory.write_b8(VM.herePP(1), 0, signed=False)
	VM.memory.write_b8(VM.herePP(1), 0, signed=False)
	# Helper to dump the bytes of the entry
	#print(binascii.hexlify(VM.memory[VM.latest:VM.here]).decode("utf-8"))
		
def defcode(VM, name, tokens, immediate=False):
	header(VM, name, immediate=immediate)
	# Needed to return head of code field
	cwa = VM.tcb.here
	# n*u16code list
	for token in tokens: VM.memory.write_b16(VM.herePP(2), token, signed=True if token<0 else False);
	# Update code length field
	VM.memory.write_b8(VM.tcb.latest+2, 2*len(tokens), signed=False)
	#print(f"Defined {(10*' '+name)[-10:]}, from {(2*'0'+hex(old)[2:])[-2:]:2}..{(2*'0'+hex(VM.here)[2:])[-2:]:2}; strlen {len(name):2}, memlen is {VM.here-old:2}")
	return cwa
