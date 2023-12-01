import functools as _f
import binascii

class Stack:
	def __init__(self, VM, bsp):
		self.VM = VM
		self.bsp = self.sp = bsp
	def push(self, bytes): 
		for byte in bytes:
			self.VM.memory[self.sp] = byte
			self.sp -= 1
	def pop(self, count):
		self.sp += count
		ret = self.VM.memory[self.sp:self.sp-count:-1]
		self.VM.memory[self.sp:self.sp-count:-1]=[0]*count
		return ret
	def dump(self):
		self.VM.dump(self.sp, self.bsp)
def write_u8(VM, address, value): VM.memory[address] = 255 & value
def write_u16(VM, address, value):
	bytes = value.to_bytes(2, "little")
	VM.memory[address:address+2]=bytes
def read_u8(VM, address):
	return int.from_bytes(VM.memory[address:address+1],"little")
def read_u16(VM, address):
	return int.from_bytes(VM.memory[address:address+2],"little")
	
def addDictEntry(VM, name, tokens):
	newTail = VM.here
	# u16 -- link
	write_u16(VM, VM.here, VM.dictTail); VM.here+=2
	VM.dictTail = newTail
	# u8 -- flags
	write_u8(VM, VM.here, 0); VM.here += 1
	# ?? I would like to encode length of block so I can dump code!!
	# u8 -- length of name.
	write_u8(VM, VM.here, len(name)); VM.here += 1
	# n*u8 string + 1/3? u8 of null
	for letter in bytearray(name, "utf-8"):
		write_u8(VM, VM.here, letter); VM.here += 1
	# Always null terminate strings, and place words on a 16b boundary
	# So, pad evens with 2*null, odds with 1.
	if VM.here % 2 == 0:
		write_u8(VM, VM.here, 0); VM.here += 1
	write_u8(VM, VM.here, 0); VM.here += 1
	# n*u16code list
	for token in tokens: write_u16(VM, VM.here, token)
	
def dump_dict(VM, dictTail): pass
		
class vm (object):
	def __init__(self):
		self.memory = bytearray(256)
		#0..128 is for dict and return stack, 128...224 is param stack, with lower addresses being PAD.
		self.rStack = Stack(self, 128)
		self.pStack = Stack(self, 224)
		self.words = []
		# Address of "previous"  dictionary entry
		self.dictTail = 0
		# Address of next available byte in dictionary memory.
		self.here = 0
	
	# Negative token numbers are native, positive token numbers are FORTH
	def nativeWord(self, name, call):
		token = len(self.words)
		self.words.append(call)
		addDictEntry(self, name, [token])
		return -(1+token)
		
	def intWord(self, name, tokens):
		addDictEntry(self, name, tokens)
		
	def execute(self, token):
		word = self.words[-token-1]
		word()
		# Need to catch Stack exceptions (underflow, overflow, etc)
		
	def dump(self, lo, hi):
		print(lo, hi)
		print(str(binascii.hexlify(self.memory[lo:hi])))
		
def docol(VM): pass
	
def dup(VM):
	top_2 = VM.pStack.pop(2)
	VM.pStack.push(top_2)
	VM.pStack.push(top_2)

def plus_i16(VM):
	lhs = int.from_bytes(VM.pStack.pop(2), "little", signed=True)
	rhs = int.from_bytes(VM.pStack.pop(2), "little", signed=True)
	VM.pStack.push((rhs+lhs).to_bytes(2, "little"))
	
def wd_tail(VM):
	VM.pStack.push(VM.dictTail.from_bytes(2, "little"))
	
def dot(VM):
	v = VM.pStack.pop(2)
	print(hex(int.from_bytes(v, "little")))
	
# ( addr -- value) # Dereference a pointer
def _q(VM):
	addr  = int.from_bytes(VM.pStack.pop(2), "little")
	value = read_u16(VM, addr)
	VM.pStack.push(value.to_bytes(2, "little"))
		
# ( addr -- ) # Prints a null terminated string starting at address
def printstr(VM):
	addr = VM.pStack.pop(2)
	addr = int.from_bytes(addr, "little")
	while (ch:=read_u8(VM, addr))  != 0: 
		print(chr(ch), end="")
		addr += 1

def bootstrap(VM):
	VM.pStack.push([6, 7])
	VM.nativeWord("CR", lambda: print())
	VM.dump(3, 20)
	token_plus_i16 = VM.nativeWord("+", _f.partial(plus_i16, VM))
	token_dot = VM.nativeWord(".", _f.partial(dot, VM))
	token_q = VM.nativeWord("?", _f.partial(_q, VM))
	token_dup = VM.nativeWord("DUP", _f.partial(dup, VM))
	token_printstr = VM.nativeWord("prntstr", _f.partial(printstr, VM))
	token_dictTail = VM.nativeWord("wd.tail", _f.partial(wd_tail, VM))
	#VM.intWord("wde.link", ["HEXPRINT"])
	#VM.intWord("wde.name", ["3", "+", "prntstr"])
	# Need to know code len first.
	#VM.intWord("wde.code", [""])
	#VM.intWord("wde.dump", ["DUP", "wde.link", "DUP", "wde.name"])
	VM.pStack.push([0x04, 0x00])
	VM.execute(token_dup)
	VM.execute(token_q)
	VM.execute(token_dot)
	VM.execute(token_printstr)
	VM.execute(token_dup)
	VM.execute(token_plus_i16)
	VM.execute(token_dot)
	
VM = vm()
bootstrap(VM)
