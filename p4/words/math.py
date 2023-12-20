from ..utils import NATIVE

# (n1 n2 -- n1+n2)
@NATIVE("+")
def plus_i16(VM):
	rhs, lhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs+rhs, signed=True)
	VM.next()

# (n1 n2 -- n1|n2)
@NATIVE("OR")
def or_i16(VM):
	rhs, lhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs | rhs, signed=True)
	VM.next()

# (n1 n2 -- n1 && n2)
@NATIVE("AND")
def and_i16(VM):
	rhs, lhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs & rhs, signed=True)
	VM.next()

# (n1 n2 -- n1 == n2)
@NATIVE("=")
def equals_i16(VM):
	rhs, lhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs == rhs, signed=True)
	VM.next()

# (n1 -- n1 > 0)
@NATIVE(">0")
def gt0_i16(VM):
	lhs = VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs > 0, signed=True)
	VM.next()

# (byte -- sign-extended word)
@NATIVE("se")
def se(VM):
	VM.pStack.push_b16(VM.pStack.pop_b8(signed=True), signed=True)
	VM.next()

# (cell -- byte) Truncate cell to byte, keeping sign (I hope)
@NATIVE("TRUNC")
def TRUNC(VM):
	val = VM.pStack.pop_b16(signed=True) & 0xff
	VM.pStack.push_b8(val, signed=False if val > 0 else True)
	VM.next()