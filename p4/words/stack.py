from ..utils import NATIVE
# ( n1 -- n1 n1 ) # Duplicate top entry of stack
@NATIVE("DUP")
def dup(VM):
	top_2 = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(top_2, signed=False)
	VM.pStack.push_b16(top_2, signed=False)
	VM.next()
	
# ( n1 -- ) # Remove top cell of stack
@NATIVE("DROP")
def drop(VM):
	VM.pStack.pop_b16(signed=False)
	VM.next()

# (  -- R0 ) # Push return stack base pointer onto TOS
@NATIVE("R0")
def r0(VM):
	VM.pStack.push_b16(VM.tcb.r0(), signed=False)
	VM.next()

# (  -- P0 ) # Push parameter stack base pointer onto TOS
@NATIVE("P0")
def p0(VM):
	VM.pStack.push_b16(VM.tcb.p0(), signed=False)
	VM.next()
		
# ( newToS --) Store ToS as new return stack pointer
@NATIVE("RSP!")
def rsp_store(VM):
	VM.tcb.rsp(VM.pStack.pop_16(signed=False))
	VM.next()
	
# ( newToS --) Store ToS as new paramter stack pointer
@NATIVE("PSP!")
def psp_store(VM):
	VM.tcb.psp(VM.pStack.pop_16(signed=False))
	VM.next()
