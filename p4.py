import enum
import binascii
class Memory:
	def __init__(self, count):
		self.memory = bytearray(count)

	# External interface
	def read_b8(self, addr, signed=False): return self.read_n(addr, 1, signed=signed)
	def read_b16(self, addr, signed=False): return self.read_n(addr, 2, signed=signed)
	def write_b8(self, addr, number, signed=False): return self.write_n(addr, 1, number, signed=signed)
	def write_b16(self, addr, number, signed=False): return self.write_n(addr, 2, number, signed=signed)
	def write_bytes(self, addr, bytes): raise NotImplementedError()
	# Used to make memory subscriptable
	def __getitem__(self, index): return self.memory[index]
	def __setitem__(self, index, value): self.memory[index] = value
	# Meant to be used internally
	def read_n(self, addr, length, signed=False): return int.from_bytes(self[addr:addr+length],"big", signed=signed)
	def write_n(self, addr, length, number, signed=False): self[addr:addr+length] = number.to_bytes(length, "big", signed=signed)
	def dump(self, lo, hi): print(str(binascii.hexlify(self.memory[lo:hi])))
	
# From: https://stackoverflow.com/a/14329943
def bytes(intVal):
	return (intVal.bitLength() + 7) // 8
	
class Stack:
	# SP is a function that does one of two things.
	# If called with no args, returns the current stack pointer.
	# Otherwise, the value will be added to the SP, and the new SP will be returned.
	def __init__(self, memory, sp, limit=lambda: 0):
		self.memory = memory
		self.sp=sp
		self.limit = limit
	def push_b8(self, number, signed=False): self.push_int(number, 1, signed=signed)
	def push_b16(self, number, signed=False): self.push_int(number, 2, signed=signed)
	def push_int(self, number, length, signed=False): 
		if self.limit()>self.sp()+length: raise Exception("Stack Overflow")
		# Swap byte order since stack is backwards
		self.sp(-length)
		self.memory[self.sp():self.sp()+length] = number.to_bytes(length, "big")

	def pop_b8(self, signed=False): return self.pop_int(1, signed=signed)
	def pop_b16(self, signed=False): return self.pop_int(2, signed=signed)
	def pop_int(self, length, signed=False):
		ret = int.from_bytes(self.memory[self.sp():self.sp() + length], "big", signed=signed)
		self.sp(length)
		return ret
		
	def push_bytes(self, bytes):
		# print(self.limit(), self.sp(), len(bytes))
		if self.limit()>self.sp()-len(bytes): raise Exception("Stack Overflow")
		for byte in bytes[::-1]:
			self.sp(-1)
			self.memory[self.sp()] = byte		
	def pop_bytes(self, count):
		sp = self.sp()
		if self.bsp<sp+count: raise Exception("Stack Underflow") 
		ret = self.memory[sp:sp+count]
		self.memory[sp:sp+count]=[0]*count
		self.sp_accessor(count)
		return ret
		
	def dump(self):
		self.memory.dump(self.sp, self.bsp)
			
class DictAccessor:
	IMMEDIATE = 0x80
	HIDDEN = 0x20
	LEN = 0x1f
	
	def __init__(self, VM):
		self.VM = VM
		
	@staticmethod
	def header(VM, name, immediate=False, hidden=False):
		# u16 (link)
		VM.memory.write_b16(VM.herePP(2), VM.tcb.latest, signed=False);
		VM.tcb.latest = VM.tcb.here - 2
		# u8 (number of token bytes)
		VM.memory.write_b8(VM.herePP(1), 0, signed=False)
		# u8 (flags | string length)
		VM.memory.write_b8(VM.herePP(1), 
			(DictAccessor.IMMEDIATE if immediate else 0)
			| (DictAccessor.HIDDEN if hidden else 0)
			| (len(name) & DictAccessor.LEN), signed=False)
		# n * u8 name string; plus 1 or 2 u8 of null
		for letter in bytearray(name, "utf-8"): VM.memory.write_b8(VM.herePP(1), letter, signed=False)
		# Always null terminate strings, and place next words on a 16b boundary
		# So, pad evens with 2*null, odds with 1.
		if VM.tcb.here % 2 == 0: VM.memory.write_b8(VM.herePP(1), 0, signed=False)
		VM.memory.write_b8(VM.herePP(1), 0, signed=False)
		# Helper to dump the bytes of the entry
		#print(binascii.hexlify(VM.memory[VM.latest:VM.here]).decode("utf-8"))
		
	@staticmethod
	def defcode(VM, name, tokens, immediate=False):
		DictAccessor.header(VM, name, immediate=immediate)
		# Needed to return head of code field
		cwa = VM.tcb.here
		# n*u16code list
		for token in tokens: VM.memory.write_b16(VM.herePP(2), token, signed=True if token<0 else False);
		# Update code length field
		VM.memory.write_b8(VM.tcb.latest+2, 2*len(tokens), signed=False)
		#print(f"Defined {(10*' '+name)[-10:]}, from {(2*'0'+hex(old)[2:])[-2:]:2}..{(2*'0'+hex(VM.here)[2:])[-2:]:2}; strlen {len(name):2}, memlen is {VM.here-old:2}")
		return cwa
		
	# Yields a pointer into the dict
	def find(self, name): pass
	def entry(self, address):
		ret = {}
		ret["link"] = VM.memory.read_b16(address, signed=False); address += 2
		ret["codelen"] = VM.memory.read_b8(address, signed=False); address += 1
		ret["strlen"] = VM.memory.read_b8(address, signed=False) & DictAccessor.LEN; address += 1
		ret["str"] = address
		# Add 2 and mask out low bit to get actual length of string
		# EVEN: 2 => 4 => 4, which is right because we pad evens with 2 nulls
		# ODD: 1 =>3 => 2, which is right because we pad odds with 1 null
		ret["cwa"] = address + ((ret["strlen"] + 2) & 0xFE)
		return ret
		
	def dump(self): 	
		current, prev = self.VM.tcb.latest, 0
		while	prev != current:
			entry = self.entry(current)
			cwa = entry["cwa"]
			exec_token = self.VM.memory.read_b16(cwa, signed=True)
		
			strs = []
			for i in range(entry["codelen"]): strs.append((4*"0" + hex(self.VM.memory.read_b16(cwa+2*i,signed=False))[2:])[-4:])
			#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
			print(f"{hex(entry['link']):4} <= {hex(entry['str']-4):4} {entry['strlen']:2}{readStr(VM,entry['str']):10} *[{hex(cwa):4}]={' '.join(strs)}")
			# Dump the dict entry in binary
			#s=binascii.hexlify(VM.memory[entry['str']-4:cwa + 2*entry["flag"]]).decode("utf-8")
			#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
			prev, current = current, entry["link"]
		
# Helper for formatting a 2 byte hex value from int		
as_hex = lambda as_hex : f"{(4*'0' + hex(as_hex)[2:])[-4:]}"

class TaskControlBlock:	
	def __init__(self):
		# Dictionary entries
		self.here = self.latest = 0
		# Instruction pointers
		self.currentWord = self.nextWord = 0
		# Parameter stack pointers
		self.psp = self.s0 = 0
		# Return stack pointers
		self.rsp = 0
	
	def psp_helper(self, arg=None):
		if arg is not None: self.psp += arg
		return self.psp
			
	def rsp_helper(self, arg=None):
		if arg is not None: self.rsp += arg
		return self.rsp
							
class vm (object):
	def __init__(self):
		self.dict = DictAccessor(self)
		self.tcb = TaskControlBlock()
		self.tcb.psp = self.tcb.s0 = 240
		self.tcb.rsp = 200
		self.memory = Memory(256)
		self.rStack = Stack(self.memory, self.tcb.rsp_helper, lambda: self.tcb.here)
		self.pStack = Stack(self.memory, self.tcb.psp_helper, lambda: self.tcb.rsp)
		self.words = []
		self.alive = True
		
	def next(self):
		self.tcb.currentWord = self.tcb.nextWord
		self.tcb.nextWord += 2	
	
	def herePP(self, incr):
		here, self.tcb.here = self.tcb.here, self.tcb.here+incr
		return here
		
	# Negative token numbers are native, positive token numbers are FORTH
	def nativeWord(self, name, call):
		token = -len(self.words)-1
		self.words.append(call)
		return DictAccessor.defcode(self, name, [token]), token
		
	def intWord(self, name, tokens):
		return DictAccessor.defcode(self, name, tokens)
		
	def step(self):
		cwa_exec = self.memory.read_b16(self.tcb.currentWord, signed=False)
		token_exec = self.memory.read_b16(cwa_exec, signed = True)
		#print(f"CWA is 0x{as_hex(self.currentWord)}, word to execute is [{as_hex(cwa_exec)}]={token_exec}")
		word = self.words[-token_exec-1]
		word(self)
		
	def run(self):
		while self.alive: self.step()
		
	def addr_from_name(self, name):
		current = self.tcb.latest
		while	current != 0:
			entry = self.dict.entry(current)
			current = entry["link"]
			entryName = readStr(self, entry["str"])
			if name == entryName: return entry
		return None
				
def NEXT(function):
	return lambda VM: (function(VM), VM.next())

@NEXT
def docol(VM):
	VM.rStack.push_b16(VM.tcb.currentWord+2, signed=False)
	VM.nextWord = VM.memory.read_b16(VM.tcb.currentWord, signed=False) + 2

# ( n1 -- n1 n1 ) # Duplicate top entry of stack
@NEXT
def dup(VM):
	top_2 = VM.pStack.pop_b16(2, signed=False)
	VM.pStack.push_b16(top_2, signed=False)
	VM.pStack.push_b16(top_2, signed=False)

@NEXT
def plus_i16(VM):
	lhs, rhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs+rhs, signed=True)

@NEXT	
def wd_tail(VM):
	VM.pStack.push_b16(VM.latest, 2, signed=False)

	
# ( n1 -- ) # Print the top value on the stack to stdout
@NEXT
def dot(VM):
	v = VM.pStack.pop_b16(signed=False)
	print(hex(v), end="")
	
# ( addr -- value) # Dereference a pointer
@NEXT
def _q(VM):
	addr  = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(VM.memory.read_u16(addr), signed=True)

@NEXT
def halt(VM):
	VM.alive = False
	print("\nHALTING")		

def readStr(VM, addr):
	outStr = ""
	while (ch:=VM.memory.read_b8(addr, signed=False))  != 0: 
		outStr += chr(ch)
		addr += 1
	return outStr
	
# ( addr -- ) # Prints a null terminated string starting at address
@NEXT
def printstr(VM):
	addr = VM.pStack.pop_b16(signed=False)
	print(readStr(VM, addr), end="")

@NEXT
def exit(VM):
	VM.nextWord = VM.rStack.pop_b16(signed=False)

@NEXT	
def plus3(VM):
	VM.pStack.push_b16(VM.pStack.pop_b16(signed=True)+3, signed=True)

@NEXT
def cr(VM):
	print()

@NEXT
def literal(VM):
	number = VM.memory.read_b16(VM.tcb.nextWord, signed=False)
	VM.pStack.push_b16(number, signed=False)
	VM.tcb.nextWord += 2
			
def bootstrap(VM):
	VM.pStack.push_bytes([6, 7])
	(cwa_cr, _) = VM.nativeWord("CR", cr)
	(cwa_docol, token_docol) = VM.nativeWord("docol", docol)
	(cwa_plus_i16, _) = VM.nativeWord("+", plus_i16)
	(cwa_dot, _) = VM.nativeWord(".", dot)
	(cwa_q, _) = VM.nativeWord("?", _q)
	(cwa_exit, _) = VM.nativeWord("EXIT", exit)
	(cwa_dup, _) = VM.nativeWord("DUP", dup)
	(cwa_printstr, _) = VM.nativeWord("prntstr", printstr)
	(cwa_latest, _) = VM.nativeWord("wd.tail", wd_tail)
	(cwa_halt, _) = VM.nativeWord("HALT", halt)
	(cwa_plus3, _) = VM.nativeWord("3+", plus3)
	(cwa_literal, _) = VM.nativeWord("LITERAL", literal)
	
	cwa_wdelink = VM.intWord("wde.link", [token_docol, cwa_q, cwa_dot, cwa_exit])
	cwa_wdename = VM.intWord("wde.name", [token_docol, cwa_plus3, cwa_printstr, cwa_exit])
	# Need to know code len first.
	#VM.intWord("wde.code", [""])
	#VM.intWord("wde.dump", ["DUP", "wde.link", "DUP", "wde.name"])
	VM.pStack.push_bytes([0x04, 0x00])
	#tokens = [cwa_latest, cwa_dup, cwa_wdelink, cwa_dup, cwa_wdename, cwa_halt]
	tokens = [cwa_literal, 0xFEED, cwa_dot, cwa_halt]
	token_exc = VM.intWord("doAll", tokens)#token_q, token_dot, token_printstr, token_halt])
	ac = DictAccessor(VM)
	ac.dump()
	
	VM.tcb.nextWord = VM.addr_from_name("doAll")["cwa"]; VM.next()
	VM.run()
	
VM = vm()
bootstrap(VM)
