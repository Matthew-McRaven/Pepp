from ..utils import NATIVE
from ..vm.sim import State as _State
from ..dictionary import find as _find, entry as _entry, Flags as _Flags
from .io import word_impl as _word_impl, number_impl as _number_impl


# ( -- ) Core REPL for my FORTH.
@NATIVE("COREINT")
def core_interpret(VM):
    word = _word_impl()
    num, isNum = _number_impl(word, 10)
    is_signed = True if num<0 else False
    if (match := _find(VM, len(word), word)) != 0:
        entry = _entry(VM, match)
        # Leave nextWord alone. Execution will resume with the following WORD then.
        if VM.tcb.state() == _State.IMMEDIATE or (entry["strlen"] & _Flags.IMMEDIATE):
            VM.ip = entry["cwa"]
        else:
            VM.memory.write_b16(VM.herePP(2), entry["cwa"])
            VM.next()
    elif isNum and (VM.tcb.state() == _State.IMMEDIATE):
        VM.pStack.push_b16(num, signed=is_signed)
        VM.next()
    elif isNum:
        lit = _entry(VM, _find(VM, len("LIT"), "LIT"))
        VM.memory.write_b16(VM.herePP(2), lit["cwa"], signed=num)
        VM.memory.write_b16(VM.herePP(2), num, signed=is_signed)
        VM.next()
    else:
        raise Exception(f"Unknown word {word}!")


# When we have a REPL, we can use this version to overwrite the "bad" version above.
# Implementation has been delayed because I am working on IF ELSE THEN
"""
: INTERPRET
WORD FIND
DUP >0 IF
    DUP ?IMMEDIATE STATE STATE_IMM = OR 
    IF
        >CWA EXECUTE
    ELSE
        >CWA ,
    THEN
    EXIT 
THEN
PREVWORD NUMBER
>0 IF 
    STATE STATE_IMM = IF
        ( NO-OP, number is already on stack)
    ELSE
        ( Push the LIT NUM into the dict entry)
        ' LIT ,
        ,
ELSE
    ." ERROR!!! " ABORT
THEN
;
"""
# Need ." " IF THEN ELSE ABORT EXECUTE ( )
