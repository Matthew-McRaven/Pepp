from ..utils import NAMED, NEXT
# Push the value stored at the next instruction pointer, and
# advance the instruction pointer by 2 words total.
@NAMED("LIT")
@NEXT
def literal(VM):
	number = VM.memory.read_b16(VM.tcb.nextWord, signed=False)
	VM.pStack.push_b16(number, signed=False)
	VM.tcb.nextWord += 2
