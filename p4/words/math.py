from ..utils import NAMED, NEXT

# (n1 n2 -- n1+n2)
@NAMED("+")
@NEXT
def plus_i16(VM):
	lhs, rhs = VM.pStack.pop_b16(signed=True), VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(lhs+rhs, signed=True)

# (n1 -- n1+3)
@NAMED("3+")
@NEXT	
def plus3_i16(VM):
	VM.pStack.push_b16(VM.pStack.pop_b16(signed=True)+3, signed=True)
