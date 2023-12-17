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

# (  -- R0 ) # Push return stack base pointer onto TOS
@NAMED("R0")
@NEXT
def r0(VM):
	VM.pStack.push_b16(VM.tcb.r0(), signed=False)

# (  -- P0 ) # Push parameter stack base pointer onto TOS
@NAMED("P0")
@NEXT
def p0(VM):
	VM.pStack.push_b16(VM.tcb.p0(), signed=False)
		
# ( newToS --) Store ToS as new return stack pointer
@NAMED("RSP!")
@NEXT
def rsp_store(VM):
	VM.tcb.rsp(VM.pStack.pop_16(signed=False))
	
# ( newToS --) Store ToS as new paramter stack pointer
@NAMED("PSP!")
@NEXT
def psp_store(VM):
	VM.tcb.psp(VM.pStack.pop_16(signed=False))
