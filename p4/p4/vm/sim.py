from .memory import *
from ..dictionary import defcode as _defcode
class TaskControlBlock:	
	def __init__(self):
		# Dictionary entries
		# Start entries at non=0, so that a dereferenced nullpointer can't clober a fundamental data structure
		self.here = 2
		# But we want that null to be the link pointer in our first entry
		# All we have to do is an 0= to check if we've arrived at dict head.
		self.latest = 0
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
		return _defcode(self, name, [token]), token
		
	def intWord(self, name, tokens):
		return _defcode(self, name, tokens)
		
	def step(self):
		cwa_exec = self.memory.read_b16(self.tcb.currentWord, signed=False)
		token_exec = self.memory.read_b16(cwa_exec, signed = True)
		#print(f"CWA is 0x{as_hex(self.currentWord)}, word to execute is [{as_hex(cwa_exec)}]={token_exec}")
		word = self.words[-token_exec-1]
		word(self)
		
	def run(self):
		while self.alive: self.step()
