import enum
import binascii

class Stack:
	def __init__(self, VM, bsp, limit=lambda: 0):
		self.VM = VM
		self.bsp = self.sp = bsp
		self.limit = limit
	def push(self, bytes): 
		if self.limit()>self.sp+len(bytes): raise Exception("Stack Overflow")
		for byte in bytes[::-1]:
			self.sp -= 1
			self.VM.memory[self.sp] = byte
	def pop(self, count):
		if self.bsp<self.sp+count: raise Exception("Stack Underflow") 
		ret = self.VM.memory[self.sp:self.sp+count]
		self.VM.memory[self.sp:self.sp+count]=[0]*count
		self.sp += count
		return ret
	def dump(self):
		self.VM.dump(self.sp, self.bsp)
		
def write_u8(VM, address, value): VM.memory[address] = 255 & value
def write_u16(VM, address, value):
	bytes = value.to_bytes(2, "big", signed=True if value <0 else False)
	VM.memory[address:address+2]=bytes
def read_u8(VM, address):
	return int.from_bytes(VM.memory[address:address+1],"big")
def read_u16(VM, address):
	return int.from_bytes(VM.memory[address:address+2],"big")
def read_i16(VM, address):
	return int.from_bytes(VM.memory[address:address+2],"big", signed=True)

class DictFlags(enum.IntEnum):
	IMMEDIATE = 0x80
	HIDDEN = 0x20
	LEN = 0x1f
		
def addDictEntry(VM, name, tokens, immediate=False, hidden=False):
	newTail = VM.here
	# u16 -- link
	write_u16(VM, VM.here, VM.dictTail); VM.here+=2
	VM.dictTail = newTail
	# u8 -- flags
	write_u8(VM, VM.here, (DictFlags.IMMEDIATE if immediate else 0)
		| (DictFlags.HIDDEN if hidden else 0)
		| len(tokens) & DictFlags.LEN); VM.here += 1
	# ?? I would like to encode length of block so I can dump code!!
	# u8 -- length of name.
	write_u8(VM, VM.here, len(name)); VM.here += 1
	# n*u8 string + 1/3? u8 of null
	# old = VM.here
	for letter in bytearray(name, "utf-8"):
		write_u8(VM, VM.here, letter); VM.here += 1
	#print(f"Defined {(10*' '+name)[-10:]}, from {(2*'0'+hex(old)[2:])[-2:]:2}..{(2*'0'+hex(VM.here)[2:])[-2:]:2}; strlen {len(name):2}, memlen is {VM.here-old:2}")
	# Always null terminate strings, and place words on a 16b boundary
	# So, pad evens with 2*null, odds with 1.
	if VM.here % 2 == 0:
		write_u8(VM, VM.here, 0); VM.here += 1
	write_u8(VM, VM.here, 0); VM.here += 1
	# n*u16code list
	cwa = VM.here
	for token in tokens: 
		write_u16(VM, VM.here, token); VM.here += 2
	return cwa
	
def getDictEntry(VM, dictTail):
	ret = {}
	ret["link"] = read_u16(VM, dictTail); dictTail += 2
	ret["flag"] = read_u8(VM, dictTail); dictTail += 1
	ret["len"] = read_u8(VM, dictTail) & DictFlags.LEN; dictTail += 1
	ret["str"] = dictTail
	# Add 2 and mask out low bit to get actual length of string
	# EVEN: 2 => 4 => 4, which is right because we pad evens with 2 nulls
	# ODD: 1 =>3 => 2, which is right because we pad odds with 1 null
	ret["cwa"] = dictTail + ((ret["len"] + 2) & 0xFE)
	return ret
	
def dumpDict(VM):
	prev, prevprev = VM.dictTail, 0
	while	prev != prevprev:
		entry = getDictEntry(VM, prev)
		cwa = entry["cwa"]
		exec_token = read_i16(VM, cwa)
		
		strs = []
		for i in range(entry["flag"]): strs.append((4*"0" + hex(read_u16(VM, cwa+2*i))[2:])[-4:])
		#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
		print(f"{hex(entry['link']):4} <= {hex(entry['str']-4):4} {entry['len']:2}{readStr(VM,entry['str']):10} *[{hex(cwa):4}]={' '.join(strs)}")
		# Dump the dict entry in binary
		#s=binascii.hexlify(VM.memory[entry['str']-4:cwa + 2*entry["flag"]]).decode("utf-8")
		#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
		prevprev = prev
		prev = entry["link"]
		
# Helper for formatting a 2 byte hex value from int		
as_hex = lambda as_hex : f"{(4*'0' + hex(as_hex)[2:])[-4:]}"
					
class vm (object):
	def __init__(self):
		self.memory = bytearray(256)
		#0..128 is for dict and return stack, 128...224 is param stack, with lower addresses being PAD.
		self.rStack = Stack(self, 200, lambda: self.here)
		self.pStack = Stack(self, 240, lambda: self.rStack.sp)
		self.words = []
		# Address of "previous"  dictionary entry
		self.dictTail = 0
		# Address of next available byte in dictionary memory.
		self.here = 0
		# Pointer to "next" word to execute, pointer to interpreter.
		self.currentWord, self.nextWord = 0, 0
		# Should execute continue?
		self.alive = True
	
	# Negative token numbers are native, positive token numbers are FORTH
	def nativeWord(self, name, call):
		token = -len(self.words)-1
		self.words.append(call)
		return addDictEntry(self, name, [token]), token
		
	def intWord(self, name, tokens):
		return addDictEntry(self, name, tokens)
		
	def step(self):
		cwa_exec = read_u16(self, self.currentWord)
		token_exec = read_i16(self, cwa_exec)
		#print(f"CWA is 0x{as_hex(self.currentWord)}, word to execute is [{as_hex(cwa_exec)}]={token_exec}")
		word = self.words[-token_exec-1]
		word(self)
		
	def run(self):
		while self.alive: self.step()
		
	def dump(self, lo, hi):
		print(str(binascii.hexlify(self.memory[lo:hi])))
		
	def addr_from_name(self, name):
		prev = self.dictTail
		while	prev != 0:
			entry = getDictEntry(self, prev)
			prev = entry["link"]
			entryName = readStr(VM, entry["str"])
			if name == entryName: return entry
		return None
			
def next(VM):
	VM.currentWord = VM.nextWord
	VM.nextWord += 2
			
def docol(VM):
	VM.rStack.push((VM.currentWord + 2).to_bytes(2, "big"))
	VM.nextWord = read_u16(VM, VM.currentWord) + 2
	next(VM)

# ( n1 -- n1 n1 ) # Duplicate top entry of stack
def dup(VM):
	top_2 = VM.pStack.pop(2)
	VM.pStack.push(top_2)
	VM.pStack.push(top_2)
	next(VM)

def plus_i16(VM):
	lhs = int.from_bytes(VM.pStack.pop(2), "big", signed=True)
	rhs = int.from_bytes(VM.pStack.pop(2), "big", signed=True)
	VM.pStack.push((rhs+lhs).to_bytes(2, "big"))
	next(VM)
	
def wd_tail(VM):
	VM.pStack.push(VM.dictTail.to_bytes(2, "big"))
	next(VM)
	
# ( n1 -- ) # Print the top value on the stack to stdout
def dot(VM):
	v = VM.pStack.pop(2)
	print(hex(int.from_bytes(v, "big")), end="")
	next(VM)
	
# ( addr -- value) # Dereference a pointer
def _q(VM):
	addr  = int.from_bytes(VM.pStack.pop(2), "big")
	value = read_u16(VM, addr)
	VM.pStack.push(value.to_bytes(2, "big"))
	next(VM)
	
def halt(VM):
	VM.alive = False
	print("\nHALTING")		
	next(VM)

def readStr(VM, addr):
	outStr = ""
	while (ch:=read_u8(VM, addr))  != 0: 
		outStr += chr(ch)
		addr += 1
	return outStr
	
# ( addr -- ) # Prints a null terminated string starting at address
def printstr(VM):
	addr = VM.pStack.pop(2)
	addr = int.from_bytes(addr, "big")
	print(readStr(VM, addr), end="")
	next(VM)
	
def exit(VM):
	VM.nextWord = int.from_bytes(VM.rStack.pop(2), "big")
	#print(f"Current word is being popped. Pointer to {VM.nextWord}")
	next(VM)
def plus3(VM):
	opr = int.from_bytes(VM.pStack.pop(2), "big", signed=True)
	VM.pStack.push((opr+3).to_bytes(2, "big"))
	next(VM)
	
def cr(VM):
	print()
	next(VM)
		
def bootstrap(VM):
	VM.pStack.push([6, 7])
	(cwa_cr, _) = VM.nativeWord("CR", cr)
	(cwa_docol, token_docol) = VM.nativeWord("docol", docol)
	(cwa_plus_i16, _) = VM.nativeWord("+", plus_i16)
	(cwa_dot, _) = VM.nativeWord(".", dot)
	(cwa_q, _) = VM.nativeWord("?", _q)
	(cwa_exit, _) = VM.nativeWord("EXIT", exit)
	(cwa_dup, _) = VM.nativeWord("DUP", dup)
	(cwa_printstr, _) = VM.nativeWord("prntstr", printstr)
	(cwa_dictTail, _) = VM.nativeWord("wd.tail", wd_tail)
	(cwa_halt, _) = VM.nativeWord("HALT", halt)
	(cwa_plus3, _) = VM.nativeWord("3+", plus3)
	
	cwa_wdelink = VM.intWord("wde.link", [token_docol, cwa_q, cwa_dot, cwa_exit])
	cwa_wdename = VM.intWord("wde.name", [token_docol, cwa_plus3, cwa_printstr, cwa_exit])
	# Need to know code len first.
	#VM.intWord("wde.code", [""])
	#VM.intWord("wde.dump", ["DUP", "wde.link", "DUP", "wde.name"])
	VM.pStack.push([0x04, 0x00])
	tokens = [cwa_dictTail, cwa_dup, cwa_wdelink, cwa_dup, cwa_wdename, cwa_halt]
	token_exc = VM.intWord("doAll", tokens)#token_q, token_dot, token_printstr, token_halt])
	dumpDict(VM)
	
	VM.nextWord = VM.addr_from_name("doAll")["cwa"]; next(VM)
	VM.run()
	
VM = vm()
bootstrap(VM)
