from p4.utils import NATIVE

# (n1 n2 -- n1+n2)
@NATIVE("+")
def plus_i16(VM):
	rhs, lhs = VM.pStack.pop_b16(signed=False), VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16((lhs + rhs) & 0xFFFF, signed=False)
	VM.next()

# (n1 n2 -- n1-n2)
@NATIVE("-")
def sub_i16(VM):
	rhs, lhs = VM.pStack.pop_b16(signed=False), VM.pStack.pop_b16(signed=False)
	dif = (lhs + ~rhs + 1) & 0xFFFF
	VM.pStack.push_b16(dif, signed=False)
	VM.next()

# (n1 n2 -- n1|n2)
@NATIVE("OR")
def or_i16(VM):
	rhs, lhs = VM.pStack.pop_b16(signed=False), VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(lhs | rhs, signed=False)
	VM.next()

# (n1 n2 -- n1 && n2)
@NATIVE("AND")
def and_i16(VM):
	rhs, lhs = VM.pStack.pop_b16(signed=False), VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(lhs & rhs, signed=False)
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
	VM.pStack.push_b16(1 if lhs > 0 else 0, signed=True)
	VM.next()

# (n1 -- n1 == 0)
@NATIVE("0=")
def eq0_i16(VM):
	lhs = VM.pStack.pop_b16(signed=True)
	VM.pStack.push_b16(1 if lhs == 0 else 0, signed=True)
	VM.next()