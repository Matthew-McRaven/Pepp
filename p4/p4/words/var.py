from ..utils import NAMED, NEXT
# Push the value stored at the next instruction pointer, and
# advance the instruction pointer by 2 words total.
@NAMED("LIT")
@NEXT
def literal(VM):
	number = VM.memory.read_b16(VM.tcb.nextWord, signed=False)
	VM.pStack.push_b16(number, signed=False)
	VM.tcb.nextWord += 2

# ( n -- addr ) Allocates N bytes of unmanaged, unreclaimable, global memory.
# Address of first byte is pushed on stack
@NAMED("ALLOT")
@NEXT
def allot(VM):
	count = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(VM.tcb.here, signed=False)
	VM.tcb.here += count
	
