import p4.sim.io
from p4.utils import NATIVE, number_impl
import p4.strings

@NATIVE("CHAR")
def char(VM):
    next_word = VM.io.word_impl()
    VM.pStack.push_b8(ord(next_word[0]), signed=False)
    VM.next()