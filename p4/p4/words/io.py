from ..utils import NAMED, NEXT

# ( n1 -- ) # Print the top value on the stack to stdout
@NAMED(".")
@NEXT
def dot(VM):
	v = VM.pStack.pop_b16(signed=False)
	print(hex(v), end="")

# Emit a CR to stdout
@NAMED("CR")
@NEXT
def cr(VM):
	print()
	
# ( addr -- ) # Prints a null terminated string starting at address
@NAMED("PRINTSTR")
@NEXT
def printstr(VM):
	addr = VM.pStack.pop_b16(signed=False)
	print(readStr(VM, addr), end="")

# Helper class to buffer values returned from input()
class __STDIN:
	def __init__(self):
		self.buffer = ""
	def key(self):
		if self.buffer:
			ret = self.buffer[0]
			self.buffer=self.buffer[1:]
			return ret
		else:
			self.buffer = input("$")
			return self.key()
	def peek(self):
		if self.buffer: return self.buffer[0]
		else: return None

__stdin = __STDIN()

# ( -- chr ) Pushes the latest character from stdin.	
@NAMED("KEY")
@NEXT
def key(VM):
	VM.pStack.push_b8(ord(__stdin.key()), signed=False)

# ( chr -- ) Pops the top of the stack as a character and write it to stdout.
@NAMED("EMIT")
@NEXT
def emit(VM):
	print(chr(VM.pStack.pop_b8(signed=False)), end="")
