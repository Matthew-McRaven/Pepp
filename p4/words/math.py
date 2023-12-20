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
	lhs, = VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs > 0, signed=True)
	VM.next()