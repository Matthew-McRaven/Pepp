"""
Native words to manipulate the task control block
"""
from p4.utils import NATIVE


# (  -- n ) Pushes latest onto stack
@NATIVE("LATEST")
def latest(VM):
    VM.pStack.push_b16(VM.tcb.latest())
    VM.next()


# (  -- n ) Pushes here onto stack
@NATIVE("HERE")
def here(VM):
    VM.pStack.push_b16(VM.tcb.baseAddress + VM.tcb.Offsets.HERE)
    VM.next()


# (  -- n ) Pushes *here onto stack
@NATIVE("HERE@")
def here_fetch(VM):
    VM.pStack.push_b16(VM.tcb.here())
    VM.next()