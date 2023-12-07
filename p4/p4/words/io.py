from ..utils import NAMED, NEXT, PADDED
import p4.strings
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

# ( addr len -- ) Print the string pointed to by the stack
@NAMED("PRINT")
@NEXT
def _print(VM):
	len = VM.pStack.pop_b8(signed=False)
	addr = VM.pStack.pop_b16(signed=False)
	print(p4.strings.readLenStr(VM, addr, len))
			
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
			self.buffer = input("$").rstrip()+"\n"
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
	
#( -- addr len) Reads the next word from STDIN into a temp buffer, then pushes the string
@PADDED("32")
@NAMED("WORD")
@NEXT
def word(VM):
	ch, iter = __stdin.key(), 0
	while ch not in ' \r\n\t':
		VM.memory.write_b8(word.pad + iter, ord(ch), signed=False)
		ch = __stdin.key()
		iter += 1
	# Null terminate the string for our C friends.
	VM.memory.write_b8(word.pad + iter + 1, 0, signed=False)
	VM.pStack.push_b16(word.pad, signed=False)
	VM.pStack.push_b8(iter, signed=False)
