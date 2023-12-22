"""
Native words in the compilation of other words
"""

from p4.utils import NATIVE, INTERPRET
from p4.vm import State as _State

# The VM's step chases pointers, and updates IP to point to the actual address being executed.
@NATIVE("ENTER", priority=float("-inf"))
def enter(VM):
	# Preserve the next logical word to be executed.
	VM.rStack.push_b16(VM.tcb.nextWord(), signed=False)
	# However, our IP may be anywhere in memory, so we should execute the word immediately following this one
	# nextWord may not point to IP+2, especially when we are near an INTERPRET word.
	VM.tcb.nextWord(VM.ip+2)
	VM.next()

# Pop top entry of return stack and jump to it
@NATIVE("EXIT", priority=float("-inf"))
def exit(VM):
	VM.tcb.nextWord(VM.rStack.pop_b16(signed=False))
	VM.next()

# ( n -- ) Pops ToS and writes it to here
@NATIVE(",")
def comma(VM):
	number = VM.pStack.pop_b16(signed=False)
	VM.memory.write_b16(VM.tcb.here(), number)
	VM.tcb.here(VM.tcb.here() + 2)
	VM.next()

# ( -- n ) Push value for state "IMMEDIATE" onto the Stack
@NATIVE("STATE_IMM")
def state(VM):
    VM.pStack.push_b16(_State.IMMEDIATE)
    VM.next()

# ( -- n ) Push the current state of the VM onto the stack
@NATIVE("STATE")
def state(VM):
    VM.pStack.push_b16(VM.tcb.state())
    VM.next()

# Enter compilation mode
@NATIVE("[")
def lbrac(VM):
	VM.tcb.state(_State.COMPILING)
	VM.next()


# Exit compilation mode
@NATIVE("]", immediate=True)
def rbrac(VM):
	VM.tcb.state(_State.IMMEDIATE)
	VM.next()

# Fetch opcode for ENTER to avoid needless pointer chase at runtime.
colon = INTERPRET(":", "WORD CREATE LIT ENTER @ , LATEST HIDDEN [")
# Fixup Code Len for latest word
fcl = INTERPRET("FCL", "HERE@ LATEST >CWA - TRUNC LATEST >CODELEN !c")
semicolon = INTERPRET(";", "LIT EXIT @ , LATEST HIDDEN ] FCL", immediate=True)
tick = INTERPRET("'", "WORD FIND >CWA @", immediate=True)