from ..utils import NAMED, NEXT

# (n1 n2 -- n1+n2)
@NAMED("+")
@NEXT
def plus_i16(VM):
	lhs, rhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs+rhs, signed=True)

# (n1 n2 -- n1+n2)
@NAMED("OR")
@NEXT
def or_i16(VM):
	lhs, rhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs | rhs, signed=True)

# (n1 n2 -- n1 && n2)
@NAMED("AND")
@NEXT
def and_i16(VM):
	lhs, rhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs & rhs, signed=True)

# (n1 n2 -- n1 == n2)
@NAMED("=")
@NEXT
def equals_i16(VM):
	lhs, rhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs == rhs, signed=True)

# (n1 -- n1 > 0)
@NAMED(">0")
@NEXT
def gt0_i16(VM):
	lhs, = VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs > 0, signed=True)