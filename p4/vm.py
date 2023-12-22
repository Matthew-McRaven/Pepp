import binascii
import enum

import p4.sim
from p4.dictionary import defcode as _defcode, nearest_header, name
from p4.utils import as_hex as _as_hex

class State(enum.IntEnum):
	COMPILING = 1
	IMMEDIATE = 0

class vm (object):
	def __init__(self, io=None):
		self.debug = False
		self.io = io if io is not None else p4.sim.stdio()
		size = 4192
		self.memory = p4.sim.Memory(size)
		self.tcb = p4.sim.TaskControlBlock(8, self.memory)
		self.tcb.here(self.tcb.maxAddress() + 1)
		self.tcb.p0(size-100)
		self.tcb.psp(self.tcb.p0())
		self.tcb.r0(size-40)
		self.tcb.rsp(self.tcb.r0())
		self.rStack = p4.sim.Stack(self.memory, self.tcb.rsp_helper, lambda: self.tcb.p0())
		self.pStack = p4.sim.Stack(self.memory, self.tcb.psp_helper, lambda: self.tcb.here())
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

	# Return the function object for a given name
	def word_from_name(self, name):
		for word in self.words:
			if word.FORTH["name"] == name: return word
		return None

	def step(self):
		opcode = self.chase_opcode(self.ip)
		self.decode_opcode(opcode)(self)
		if self.debug: print(_as_hex(opcode), binascii.hexlify(self.rStack.bytes()))


	def decode_opcode(self, opcode):
		token = -opcode - 1
		return self.words[token]


	def chase_opcode(self, addr):
		addr_chain = [addr] if self.debug else None
		opcode = self.memory.read_b16(addr, signed=True)
		while opcode > 0:
			# Push current address onto stack to imitate DOCOL
			addr, opcode = opcode, self.memory.read_b16(opcode, signed=True)
			if self.debug: addr_chain.append(addr)
		self.ip = addr
		if self.debug:
			addr_chain_str = " = ".join([f"(*[{_as_hex(it)}])" for it in addr_chain[::-1]])
			name_chain = " ".join(f"{name(self, nearest_header(self, it))}" for it in addr_chain)
			print(f"Executing CWA {_as_hex(self.ip):4} opcode {self.decode_opcode(opcode).FORTH['name']:10} = {addr_chain_str}")
			#print(f"{self.decode_opcode(opcode).FORTH['name']:10}")
			# print(f"The return stack is {name_chain} {{{self.decode_opcode(opcode).FORTH['name']}}}")
		return opcode

	def run(self):
		while self.alive: self.step()
