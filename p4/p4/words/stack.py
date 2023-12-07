from ..utils import NAMED, NEXT
# ( n1 -- n1 n1 ) # Duplicate top entry of stack
@NAMED("DUP")
@NEXT
def dup(VM):
	top_2 = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(top_2, signed=False)
	VM.pStack.push_b16(top_2, signed=False)
	
# ( n1 -- ) # Remove top cell of stack
@NAMED("DROP")
@NEXT
def drop(VM):
	VM.pStack.pop_b16(signed=False)

