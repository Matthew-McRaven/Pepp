"""
Words to convert bytes, cells, double-cells, etc.
"""

from p4.utils import NATIVE
# (byte -- sign-extended word)
@NATIVE("SE")
def se(VM):
	VM.pStack.push_b16(VM.pStack.pop_b8(signed=True), signed=True)
	VM.next()

# (cell -- byte) Truncate cell to byte, keeping sign (I hope)
@NATIVE("TRUNC")
def TRUNC(VM):
	val = VM.pStack.pop_b16(signed=True) & 0xff
	VM.pStack.push_b8(val, signed=False if val > 0 else True)
	VM.next()