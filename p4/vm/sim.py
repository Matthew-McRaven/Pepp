from .memory import *
from ..dictionary import defcode as _defcode
from ..utils import as_hex as _as_hex

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
		self.alive = True
		
	def next(self):
		self.tcb.currentWord(self.tcb.nextWord())
		self.tcb.nextWord(self.tcb.nextWord() + 2)	
	
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
		cwa_exec = self.memory.read_b16(self.tcb.currentWord(), signed=False)
		token_exec = self.memory.read_b16(cwa_exec, signed = True)
		#print(f"CWA is 0x{_as_hex(self.tcb.currentWord())}, word to execute is [{_as_hex(cwa_exec)}]={_as_hex(token_exec & 0xFFFF)}")
		word = self.words[-token_exec-1]
		word(self)
		
	def run(self):
		while self.alive: self.step()
