from ..utils import NATIVE
from ..vm.sim import State as _State

# The VM's step chases pointers, and updates IP to point to the actuall address being executed.
@NATIVE("ENTER")
def enter(VM):
	# Preserve the next logical word to be executed.
	VM.rStack.push_b16(VM.tcb.nextWord(), signed=False)
	# However, our IP may be anywhere in memory, so we should execute the word immediately following this one
	# nextWord may not point to IP+2, especially when we are near an INTERPRET word.
	VM.tcb.nextWord(VM.ip+2)
	VM.next()

# Pop top entry of return stack and jump to it
@NATIVE("EXIT")
def exit(VM):
	VM.tcb.nextWord(VM.rStack.pop_b16(signed=False))
	VM.next()

# ( addr -- value) # Dereference a pointer
@NATIVE("@")
def fetch(VM):
	addr = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(VM.memory.read_b16(addr, False), signed=False)
	VM.next()

# ( addr -- value) # Dereference a pointer to a byte
@NATIVE("@c")
def fetchchar(VM):
	addr = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b8(VM.memory.read_b8(addr, False), signed=False)
	VM.next()

# ( addr value --) # Write to a pointer
@NATIVE("!")
def store(VM):
	value = VM.pStack.pop_b16(signed=False)
	addr = VM.pStack.pop_b16(signed=False)
	VM.memory.write_b16(addr, value, False)
	VM.next()

# ( addr value --) # Write to a pointer, single byte
@NATIVE("!c")
def storechar(VM):
	value = VM.pStack.pop_b8(signed=False)
	addr = VM.pStack.pop_b16(signed=False)
	VM.memory.write_b8(addr, value, False)
	VM.next()

# Stops further instructions from executing in VM
@NATIVE("HALT")
def halt(VM):
	VM.alive = False
	print("\nHALTING")
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


# ( -- ) Relative unconditional branch, consumes following cell for jump address.
# Do not call VM.next()
def branch_helper(VM):
	offset = VM.memory.read_b16(VM.tcb.nextWord(), signed=True)
	nextWord = VM.tcb.nextWord() + offset
	VM.tcb.nextWord(nextWord)


# ( -- ) Relative unconditional branch, consumes following cell for jump address
@NATIVE("BRANCH", immediate=True)
def branch(VM):
	branch_helper(VM)
	VM.next()
	
# ( n -- ) Relative conditional branch, consumes following cell for jump address
@NATIVE("0BRANCH", immediate=True)
def branch0(VM):
	if VM.rStack.pop_b16(signed=False) == 0: branch_helper(VM)
	else: VM.tcb.nextWord(VM.tcb.nextWord() + 2)
	VM.next()

# ( -- n ) Push the current state of the VM onto the stack
@NATIVE("STATE")
def state(VM):
	VM.pStack.push_b16(VM.tcb.state())
	VM.next()

# ( -- n ) Push value for state "IMMEDIATE" onto the Stack
@NATIVE("STATE_IMM")
def state(VM):
	VM.pStack.push_b16(_State.IMMEDIATE)
	VM.next()

# ( -- n ) Get number of bytes on parameter stack. Will likely crash on underflow.
@NATIVE("DEPTH")
def depth(VM):
	VM.pStack.push_b16(VM.tcb.p0() - VM.tcb.psp())
	VM.next()

