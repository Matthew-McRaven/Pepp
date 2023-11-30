import functools as _f
import binascii
class Stack:
	def __init__(self, size):
		self.memory = bytearray(size)
		self.sp = size-1
	def push(self, bytes): 
		for byte in bytes:
			self.memory[self.sp] = byte
			self.sp -= 1
	def pop(self, count):
		self.sp+=count
		ret=self.memory[self.sp:self.sp-count:-1]
		self.memory[self.sp:self.sp-count:-1]=[0]*count
		return ret
	def dump(self):
		print(str(binascii.hexlify(self.memory)))
		
class vm (object):
	def __init__(self):
		self.rStack = Stack(5)
		self.pStack = Stack(5)
		self.words = []
	def define(self, name, behavior):
		token = len(self.words)
		self.words.append({"name":name, "call":behavior, "token":token})
		return token
	
	def execute(self, token):
		word = self.words[token]
		word["call"]()
		# Need to catch Stack exceptions (underflow, overflow, etc)
		
	
		
def dup(VM):
	top_2 = VM.pStack.pop(2)
	VM.pStack.push(top_2)
	VM.pStack.push(top_2)

def plus_i16(VM):
	lhs = int.from_bytes(VM.pStack.pop(2), "little", signed=True)
	rhs = int.from_bytes(VM.pStack.pop(2), "little", signed=True)
	VM.pStack.push((rhs+lhs).to_bytes(2, "little"))

def dot(VM):
	v = VM.pStack.pop(2)
	print(hex(int.from_bytes(v, "little")))
def bootstrap(VM):
	VM.pStack.push([6, 7])
	VM.define("CR", lambda: print())
	token_plus_i16 = VM.define("+", _f.partial(plus_i16, VM))
	token_dot = VM.define(".", _f.partial(dot, VM))
	token_dup = VM.define("DUP", _f.partial(dup, VM))
	VM.execute(token_dup)
	VM.execute(token_plus_i16)
	VM.execute(token_dot)
	
VM = vm()
bootstrap(VM)
