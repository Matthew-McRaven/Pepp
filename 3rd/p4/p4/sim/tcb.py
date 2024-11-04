import enum

class TaskControlBlock:
	class Offsets(enum.IntEnum):
		HERE = 0
		LATEST = 2
		CURRENT_WORD = 4
		NEXT_WORD = 6
		PSP = 8
		RSP = 10
		P0 = 12
		R0 = 14
		STATE = 16

	def maxAddress(self):
		return self.baseAddress + max(TaskControlBlock.Offsets) + 1

	def __init__(self, baseAddress, memory):
		self.baseAddress = baseAddress
		self.memory = memory

		# Dictionary entries
		# Start entries at non=0, so that a dereferenced nullpointer can't clober a fundamental data structure
		self.here(2)
		# But we want that null to be the link pointer in our first entry
		# All we have to do is an 0= to check if we've arrived at dict head.
		self.latest(0)
		# Instruction pointers
		self.currentWord(0)
		self.nextWord(0)
		# Parameter stack pointers
		self.psp(0)
		self.p0(0)
		# Return stack pointers
		self.rsp(0)
		self.r0(0)
		# 0 if executing, >0 if compiling
		self.state(0)

	def here(self, value=None):
		return self.access(TaskControlBlock.Offsets.HERE, value)

	def latest(self, value=None):
		return self.access(TaskControlBlock.Offsets.LATEST, value)

	def currentWord(self, value=None):
		return self.access(TaskControlBlock.Offsets.CURRENT_WORD, value)

	def nextWord(self, value=None):
		return self.access(TaskControlBlock.Offsets.NEXT_WORD, value)

	def psp(self, value=None):
		return self.access(TaskControlBlock.Offsets.PSP, value)

	def rsp(self, value=None):
		return self.access(TaskControlBlock.Offsets.RSP, value)

	def p0(self, value=None):
		return self.access(TaskControlBlock.Offsets.P0, value)

	def r0(self, value=None):
		return self.access(TaskControlBlock.Offsets.R0, value)

	def state(self, value=None):
		return self.access(TaskControlBlock.Offsets.STATE, value)

	def access(self, offset, value=None):
		if value is None:
			return self.memory.read_b16(self.baseAddress + offset, signed=False)
		else:
			return self.memory.write_b16(self.baseAddress + offset, value, signed=False if value > 0 else True)

	def psp_helper(self, arg=None):
		if arg is not None: self.psp(self.psp() + arg)
		return self.psp()

	def rsp_helper(self, arg=None):
		if arg is not None: self.rsp(self.rsp() + arg)
		return self.rsp()
