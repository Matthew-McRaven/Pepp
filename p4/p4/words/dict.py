from ..dictionary import find as _find
from ..dictionary import Offsets as _Offsets ,Flags as _Flags
from ..dictionary import cwa as _cwa
from ..utils import NAMED, NEXT
from ..strings import readStr as _readStr
import p4.utils, p4.dictionary

# (str-addr (*u8) str-len (u8) -- {dict-ptr | 0} as u16)
@NAMED("FIND")
@NEXT
def find(VM):
	strLen = VM.pStack.pop_b8(signed=False)
	strAddr = VM.pStack.pop_b16(signed=False)
	strText = _readStr(VM, strAddr)
	print(strLen, p4.utils.as_hex(strAddr), strText)
	VM.pStack.push_b16(_find(VM, strLen, strText, matchHidden=False))

# (dict-addr {*u16} -- cwa-ptr {*u16})	
@NAMED(">CWA")
@NEXT
def cwa(VM):
	dictPtr = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(_cwa(VM, dictPtr))

# This is a DUMMY word which should probably be removed, it doesn't offer much helo
# (dict-addr {*u16} -- str-addr (*u8) str-len (u8))	
@NAMED("NAME")
@NEXT	
def name(VM):
	dictPtr = VM.pStack.pop_b16(signed=False)
	VM.pStack.push_b16(dictPtr + _Offsets.STR)
	strlen = VM.memory.read_b8(dictPtr + _Offsets.STRLEN, signed=False) & _Flags.MAX_LEN
	VM.pStack.push_b8(strlen)
	
# ( addr len -- ) Creates a new dictionary entry from a string pointer
@NAMED("CREATE")
@NEXT
def create(VM):
	len = VM.pStack.pop_b8(signed=False)
	addr = VM.pStack.pop_b16(signed=False)
	text = p4.strings.readLenStr(VM, addr, len)
	p4.dictionary.header(VM, text)
	
# ( n -- ) Pops ToS and writes it to LATEST
@NAMED("CREATE")
@NEXT
def create(VM):
	number = VM.pStack.pop_b16(signed=False)
	VM.memory.write_u16(VM.tcb.here, number)
	VM.tcb.here += 2
