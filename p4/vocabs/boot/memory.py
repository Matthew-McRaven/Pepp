"""
Words to directly read, write, and copy bytes in main memory
"""
from p4.utils import NATIVE

# ( addr -- value) # Dereference a pointer
@NATIVE("@")
def fetch(VM):
	addr = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(VM.memory.read_b16(addr, False), signed=False)
	VM.next()

# ( addr -- value) # Dereference a pointer to a byte
@NATIVE("@c")
def fetchchar(VM):
	addr = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b8(VM.memory.read_b8(addr, False), signed=False)
	VM.next()

# ( value addr--) # Write to a pointer
@NATIVE("!")
def store(VM):
	addr = VM.pStack.pop_b16(signed=False)
	value = VM.pStack.pop_b16(signed=False)
	VM.memory.write_b16(addr, value, False)
	VM.next()

# ( value addr --) # Write to a pointer, single byte
@NATIVE("!c")
def storechar(VM):
	addr = VM.pStack.pop_b16(signed=False)
	value = VM.pStack.pop_b8(signed=False)
	VM.memory.write_b8(addr, value, False)
	VM.next()