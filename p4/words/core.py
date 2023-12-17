from ..utils import NAMED, NEXT

@NAMED("DOCOL")
@NEXT
def docol(VM):
	VM.rStack.push_b16(VM.tcb.currentWord()+2, signed=False)
	VM.tcb.nextWord(VM.memory.read_b16(VM.tcb.currentWord(), signed=False) + 2)

# ( addr -- value) # Dereference a pointer
@NAMED("?")
@NEXT
def question(VM):
	addr = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(VM.memory.read_b16(addr, False), signed=False)
	
# Stops further instructions from executing in VM
@NAMED("HALT")
@NEXT
def halt(VM):
	VM.alive = False
	print("\nHALTING")
	
# Pop top entry of return stack and jump to it
@NAMED("EXIT")
@NEXT
def exit(VM):
	VM.tcb.nextWord(VM.rStack.pop_b16(signed=False))
	
# Enter compilation mode
@NAMED("[")
@NEXT
def lbrac(VM):
	VM.tcb.state(1)
	
# Exit compilation mode
@NAMED("]")
@NEXT
def rbrac(VM):
	VM.tcb.state(0)
