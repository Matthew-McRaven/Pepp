"""
Native words to insert / modify entries in the dictionary
"""

from p4.dictionary import Offsets as _Offsets, Flags as _Flags, find as _find, cwa as _cwa
from p4.utils import NATIVE, INTERPRET
from p4.strings import readStr as _readStr
import p4.utils, p4.dictionary


# (str-addr (*u8) str-len (u8) -- {dict-ptr | 0} as u16)
@NATIVE("FIND")
def find(VM):
    strLen = VM.pStack.pop_b8(signed=False)
    strAddr = VM.pStack.pop_b16(signed=False)
    strText = _readStr(VM, strAddr)
    # print(strLen, p4.utils.as_hex(strAddr), strText)
    VM.pStack.push_b16(_find(VM, strLen, strText, matchHidden=False))
    VM.next()


# (dict-addr {*u16} -- cwa-ptr {*u16})
@NATIVE(">CWA")
def cwa(VM):
    dictPtr = VM.pStack.pop_b16(signed=False)
    VM.pStack.push_b16(_cwa(VM, dictPtr))
    VM.next()


# (dict-addr {*u16} -- ptr-to- {*u16})
@NATIVE(">CODELEN")
def codelen(VM):
    dictPtr = VM.pStack.pop_b16(signed=False)
    VM.pStack.push_b16(dictPtr + _Offsets.CODELEN)
    VM.next()


# This is a DUMMY word which should probably be removed, it doesn't offer much helo
# (dict-addr {*u16} -- str-addr (*u8) str-len (u8))
@NATIVE("NAME")
def name(VM):
    dictPtr = VM.pStack.pop_b16(signed=False)
    VM.pStack.push_b16(dictPtr + _Offsets.STR)
    strlen = VM.memory.read_b8(dictPtr + _Offsets.STRLEN, signed=False) & _Flags.MAX_LEN
    VM.pStack.push_b8(strlen)
    VM.next()


# ( addr len -- ) Creates a new dictionary entry from a string pointer
@NATIVE("CREATE")
def create(VM):
    len = VM.pStack.pop_b8(signed=False)
    addr = VM.pStack.pop_b16(signed=False)
    text = p4.strings.readLenStr(VM, addr, len)
    p4.dictionary.header(VM, text)
    VM.next()


# ( addr -- ) Toggles the hidden bit for a pointer to an entry
@NATIVE("HIDDEN")
def hidden(VM):
    addr = VM.pStack.pop_b16(signed=False)
    curFlags = VM.memory.read_b8(addr + _Offsets.STRLEN)
    VM.memory.write_b8(addr + _Offsets.STRLEN, curFlags ^ _Flags.HIDDEN)
    VM.next()

hide = INTERPRET("HIDE", "WORD FIND HIDDEN", immediate=True)

# ( addr -- ) Toggles the immediate bit for a pointer to an entry
@NATIVE("IMMEDIATE", immediate=True)
def IMMEDIATE(VM):
    addr = VM.pStack.pop_b16(signed=False)
    curFlags = VM.memory.read_b8(addr + _Offsets.STRLEN)
    VM.memory.write_b8(addr + _Offsets.STRLEN, curFlags ^ _Flags.IMMEDIATE)
    VM.next()


# ( addr -- ) Returns true if the dict pointer is IMMEDIATE
@NATIVE("?IMMEDIATE")
def is_immediate(VM):
    addr = VM.pStack.pop_b16(signed=False)
    curFlags = VM.memory.read_b8(addr + _Offsets.STRLEN)
    VM.pStack.push_b16(curFlags & _Flags.IMMEDIATE)
    VM.memory.write_b8(addr + _Offsets.STRLEN, curFlags ^ _Flags.HIDDEN)
    VM.next()