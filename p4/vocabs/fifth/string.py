import p4.strings
from p4.utils import NATIVE


# ( addr1 len1 addr2 len2 -- 1 | 0 ) # 1 if the strings are equal else 0
@NATIVE("STR=")
def streq(VM):
    len2 = VM.pStack.pop_b8(signed=False)
    addr2 = VM.pStack.pop_b16(signed=False)
    str2 = p4.strings.readLenStr(VM, addr2, len2)
    len1 = VM.pStack.pop_b8(signed=False)
    addr1 = VM.pStack.pop_b16(signed=False)
    str1 = p4.strings.readLenStr(VM, addr1, len1)
    VM.pStack.push_b1(1 if str1 == str2 else 0, signed=False)
    VM.next()

# ( from to count -- ) # Copies count bytes from "from" to "to".
@NATIVE("CMOVE")
def cmove(VM):
    count = VM.pStack.pop_b16(signed=False)
    _to = VM.pStack.pop_b16(signed=False)
    _from = VM.pStack.pop_b16(signed=False)
    iter_args = None

    # If copying to a smaller address, copy from first byte in to prevent clobbering
    if _to < _from: iter_args=(0, count, 1)
    # If copying to a larger address, copy from last byte in to prevent clobbering
    else: iter_args = (count, 0, -1)

    for i in range(*iter_args): VM.memory[_to+i] = VM.memory[_from+i]
    VM.next()

