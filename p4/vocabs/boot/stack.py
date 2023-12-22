"""
Low-level native words to manipulate data/order on the stack
"""

from p4.utils import NATIVE


# ( n1 n2 -- n1 n2 n1 ) #
@NATIVE("OVER")
def over(VM):
    top, bottom = VM.pStack.pop_b16(), VM.pStack.pop_b16()
    VM.pStack.push_b16(bottom)
    VM.pStack.push_b16(top)
    VM.pStack.push_b16(bottom)
    VM.next()

# ( n1 n2 -- n2 n1 n2 ) #
@NATIVE("TUCK")
def tuck(VM):
    top, bottom = VM.pStack.pop_b16(), VM.pStack.pop_b16()
    VM.pStack.push_b16(top)
    VM.pStack.push_b16(bottom)
    VM.pStack.push_b16(top)
    VM.next()

    
# Push the value stored at the next instruction pointer, and
# advance the instruction pointer by 2 words total.
@NATIVE("LIT")
def literal(VM):
    number = VM.memory.read_b16(VM.tcb.nextWord(), signed=False)
    VM.pStack.push_b16(number, signed=False)
    VM.tcb.nextWord(VM.tcb.nextWord() + 2)
    VM.next()

# Push the value stored at the next instruction pointer, and
# advance the instruction pointer by 4 bytes total.
@NATIVE("LIT.u8")
def literal_u8(VM):
    number = VM.memory.read_b16(VM.tcb.nextWord(), signed=False)
    VM.pStack.push_b8(number & 0xff, signed=False)
    VM.tcb.nextWord(VM.tcb.nextWord() + 2)
    VM.next()

# ( n1 -- n1 n1 ) # Duplicate top entry of stack
@NATIVE("DUP")
def dup(VM):
    top_2 = VM.pStack.pop_b16(signed=False)
    VM.pStack.push_b16(top_2, signed=False)
    VM.pStack.push_b16(top_2, signed=False)
    VM.next()


# ( n1 n2 -- n2 n1 ) # Duplicate top entry of stack
@NATIVE("SWAP")
def swap(VM):
    _1, _2 = VM.pStack.pop_b16(signed=False), VM.pStack.pop_b16(signed=False)
    VM.pStack.push_b16(_1, signed=False)
    VM.pStack.push_b16(_2, signed=False)
    VM.next()


# ( n1 -- ) # Remove top cell of stack
@NATIVE("DROP")
def drop(VM):
    VM.pStack.pop_b16(signed=False)
    VM.next()


# ( -- n ) Get number of bytes on parameter stack. Will likely crash on underflow.
@NATIVE("DEPTH")
def depth(VM):
    VM.pStack.push_b16(VM.tcb.p0() - VM.tcb.psp())
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

# ( n -- ) Transfer the top word from the param stack to the return stack
@NATIVE(">R")
def from_ps_to_rs(VM):
    VM.rStack.push_b16(VM.pStack.pop_b16())
    VM.next()

# ( -- n ) Transfer the top word of return stack to param stack
#: test 2 ;
@NATIVE("R>")
def from_rs_to_ps(VM):
    VM.pStack.push_b16(VM.rStack.pop_b16())
    VM.next()