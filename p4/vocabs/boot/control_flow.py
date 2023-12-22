"""
Native words for manipulating the control flow of the VM
"""

from p4.utils import NATIVE

# Stops further instructions from executing in VM
@NATIVE("HALT")
def halt(VM):
    VM.alive = False
    VM.io.log.critical("\nHALTING")
    VM.next()


# ( -- ) Relative unconditional branch, consumes following cell for jump address.
# Do not call VM.next()
def branch_helper(VM):
    offset = VM.memory.read_b16(VM.tcb.nextWord(), signed=True)
    nextWord = VM.tcb.nextWord() + offset
    VM.tcb.nextWord(nextWord)


# ( -- ) Relative unconditional branch, consumes following cell for jump address
@NATIVE("BRANCH")
def branch(VM):
    branch_helper(VM)
    VM.next()


# ( n -- ) Relative conditional branch, consumes following cell for jump address
@NATIVE("0BRANCH")
def branch0(VM):
    if VM.pStack.pop_b16(signed=False) == 0:
        branch_helper(VM)
    else:
        VM.tcb.nextWord(VM.tcb.nextWord() + 2)
    VM.next()