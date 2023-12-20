from ..io import stdin
from ..utils import NATIVE, number_impl
import p4.strings
# ( n1 -- ) # Print the top value on the stack to stdout
@NATIVE(".")
def dot(VM):
	v = VM.pStack.pop_b16(signed=False)
	print(hex(v), end="")
	VM.next()

# Emit a CR to stdout
@NATIVE("CR")
def cr(VM):
	print()
	VM.next()

# ( addr len -- ) Print the string pointed to by the stack
@NATIVE("PRINT")
def _print(VM):
	len = VM.pStack.pop_b8(signed=False)
	addr = VM.pStack.pop_b16(signed=False)
	print(p4.strings.readLenStr(VM, addr, len))
	VM.next()
			
# ( addr -- ) # Prints a null terminated string starting at address
@NATIVE("PRINTSTR")
def printstr(VM):
	addr = VM.pStack.pop_b16(signed=False)
	print(p4.strings.readStr(VM, addr), end="")
	VM.next()

# ( -- chr ) Pushes the latest character from stdin.	
@NATIVE("KEY")
def key(VM):
	VM.pStack.push_b8(ord(stdin().key()), signed=False)
	VM.next()

# ( chr -- ) Pops the top of the stack as a character and write it to stdout.
@NATIVE("EMIT")
def emit(VM):
	print(chr(VM.pStack.pop_b8(signed=False)), end="")
	VM.next()

#( -- addr len) Reads the next word from STDIN into a temp buffer, then pushes the string
@NATIVE("WORD", pad=33)
def word(VM):
	string = word_impl()
	for idx, ch in enumerate(string):
		VM.memory.write_b8(word.pad+idx+1, ord(ch), signed=False)
	# Write the length to the first byte of the pad
	VM.memory.write_b8(word.pad, len(string), signed=False)
	# Null terminate the string for our C friends.
	VM.memory.write_b8(word.pad + len(string) + 1, 0, signed=False)
	VM.pStack.push_b16(word.pad + 1, signed=False)
	VM.pStack.push_b8(len(string), signed=False)
	VM.next()

#( -- addr len) Return the length and address of WORD's currently buffered string
@NATIVE("PREVWORD", refs=["WORD"])
def prevword(VM):
	word_ref = prevword.FORTH["refs"]["WORD"]
	VM.pStack.push_b16(word_ref.pad + 1, signed=False)
	VM.pStack.push_b8(VM.memory.read_b8(word_ref.pad, signed=False), signed=False)
	VM.next()

# TODO: Allow base to vary
# ( addr len -- n 1u16 | 0u16 0u16) Parse the pointed number in the current base
# If success, push the number onto the stack, and a true flag. Otherwise both are 0.
@NATIVE("NUMBER")
def _number(VM):
	strlen = VM.pStack.pop_b8(signed=False)
	addr = VM.pStack.pop_b16(signed=False)
	text = p4.strings.readLenStr(VM, addr, strlen)
	number, flag = number_impl(text, 10)
	VM.pStack.push_b16(number, signed=False if number > 0 else True)
	VM.pStack.push_b16(flag, signed=False)
	VM.next()

	
