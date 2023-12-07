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
