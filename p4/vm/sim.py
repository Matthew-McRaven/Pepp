import enum

from .memory import *
from ..dictionary import defcode as _defcode, nearest_header, name
from ..utils import as_hex as _as_hex

class State(enum.IntEnum):
	COMPILING = 1
	IMMEDIATE = 0
class vm (object):
	def __init__(self):
		self.memory = Memory(1024)
		self.tcb = TaskControlBlock(8, self.memory)
		self.tcb.here(self.tcb.maxAddress() + 1)
		self.tcb.p0(950)
		self.tcb.psp(self.tcb.p0())
		self.tcb.r0(990)
		self.tcb.rsp(self.tcb.r0())
		self.rStack = Stack(self.memory, self.tcb.rsp_helper, lambda: self.tcb.p0())
		self.pStack = Stack(self.memory, self.tcb.psp_helper, lambda: self.tcb.here())
		self.words = []
		# The actual instruction pointer of the VM
		# Usually, this value will be updated by next() from currentWord and nextWord
		# However, when you need to (temporarily) transfer control without changing control flow (i.e., DOCOL),
		# you can update this value directly.
		self.ip = 0
		self.alive = True
		
	def next(self):
		self.tcb.currentWord(self.tcb.nextWord())
		self.tcb.nextWord(self.tcb.nextWord() + 2)
		self.ip = self.tcb.currentWord()
	
	def herePP(self, incr):
		here = self.tcb.here()
		self.tcb.here(here + incr)
		return here
		
	# Negative token numbers are native, positive token numbers are FORTH
	def nativeWord(self, name, call, immediate=False):
		token = -len(self.words)-1
		self.words.append(call)
		return _defcode(self, name, [token], immediate=immediate), token
		
	def step(self):
		opcode = self.chase_opcode(self.ip)
		self.decode_opcode(opcode)(self)


	def decode_opcode(self, opcode):
		token = -opcode - 1
		return self.words[token]


	def chase_opcode(self, addr, debug=False):
		debug = False
		addr_chain = [addr] if debug else None
		opcode = self.memory.read_b16(addr, signed=True)
		while opcode > 0:
			# Push current address onto stack to imitate DOCOL
			addr, opcode = opcode, self.memory.read_b16(opcode, signed=True)
			if debug: addr_chain.append(addr)
		self.ip = addr
		if debug:
			addr_chain_str = " = ".join([f"(*[{_as_hex(it)}])" for it in addr_chain[::-1]])
			name_chain = " ".join(f"{name(self, nearest_header(self, it))}" for it in addr_chain)
			print(f"Executing CWA {_as_hex(self.ip):4} opcode {self.decode_opcode(opcode).FORTH['name']:10} = {addr_chain_str}")
			print(f"The return stack is {name_chain} {{{self.decode_opcode(opcode).FORTH['name']}}}")
		return opcode

	def run(self):
		while self.alive: self.step()
