import enum

from .memory import *
from ..dictionary import defcode as _defcode
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
		# Address to which we will be jumping next. Usually set via next(),
		# but native words like interpret may modify this to change jump target without modifying instruction pointer.
		self.jumpTo = 0
		self.alive = True
		
	def next(self):
		self.tcb.currentWord(self.tcb.nextWord())
		self.tcb.nextWord(self.tcb.nextWord() + 2)
		self.jumpTo = self.tcb.currentWord()
	
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
		lookup = lambda addr: self.memory.read_b16(addr, signed=True)
		addr = self.jumpTo
		opcode = lookup(addr)
		while opcode > 0:
			# Push current address onto stack to imitate DOCOL
			self.rStack.push_b16(addr)
			addr, opcode = opcode, lookup(opcode)
		token = -opcode - 1
		word = self.words[token]
		#print(f"CWA is 0x{_as_hex(self.jumpTo)},  executing opcode at [{_as_hex(addr)}]={_as_hex(opcode & 0xFFFF)}")
		word(self)


		
	def run(self):
		while self.alive: self.step()
