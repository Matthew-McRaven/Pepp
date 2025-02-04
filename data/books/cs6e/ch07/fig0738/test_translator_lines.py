from io import StringIO
from typing import cast

from lang.arguments import IdentifierArgument, IntegerArgument
from lang.code import UnaryInstr, Error, OneArgInstr, TwoArgInstr
from lang.mnemonics import Mnemonics
from lang.translator import Translator


def test_0arity():
    tr = Translator(StringIO("STOP\n  END "))
    it = tr.__iter__()
    assert type(code := next(it)) is UnaryInstr
    assert cast(UnaryInstr, code).mnemonic == Mnemonics.STOP

    assert type(code := next(it)) is UnaryInstr
    assert cast(UnaryInstr, code).mnemonic == Mnemonics.END


def test_fake_0arity():
    tr = Translator(StringIO("STOPS\n"))
    it = tr.__iter__()
    assert type(code := next(it)) is Error


def test_1arity():
    tr = Translator(StringIO("ABS(X)\n  NEG(F) "))
    it = tr.__iter__()
    assert type(code := next(it)) is OneArgInstr
    assert cast(OneArgInstr, code).mnemonic == Mnemonics.ABS
    assert type(arg := cast(OneArgInstr, code).arg) == IdentifierArgument
    assert arg.generate_code() == "X"

    assert type(code := next(it)) is OneArgInstr
    assert cast(OneArgInstr, code).mnemonic == Mnemonics.NEG
    assert type(arg := cast(OneArgInstr, code).arg) == IdentifierArgument
    assert arg.generate_code() == "F"


def test_fake_1arity():
    tr = Translator(StringIO("ABSED(X) "))
    it = tr.__iter__()
    assert type(code := next(it)) is Error


def test_no_close_1arity():
    tr = Translator(StringIO("ABS(X "))
    it = tr.__iter__()
    assert type(code := next(it)) is Error


def test_2arity():
    tr = Translator(StringIO("SET(X,4)\nADD(Y, 2)\nMUL(X,X)\nDIV(X,-1)\nSUB(X,Y) "))
    it = tr.__iter__()
    assert type(code := next(it)) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.SET
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "X"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IntegerArgument
    assert arg.generate_code() == "4"

    assert type(code := next(it)) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.ADD
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "Y"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IntegerArgument
    assert arg.generate_code() == "2"

    assert type(code := next(it)) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.MUL
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "X"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IdentifierArgument
    assert arg.generate_code() == "X"

    assert type(code := next(it)) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.DIV
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "X"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IntegerArgument
    assert arg.generate_code() == "-1"

    assert type(code := next(it)) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.SUB
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "X"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IdentifierArgument
    assert arg.generate_code() == "Y"


def test_noident_2arity():
    tr = Translator(StringIO("SET(4,4) "))
    it = tr.__iter__()
    assert type(code := next(it)) is Error


def test_missing_comma_2arity():
    tr = Translator(StringIO("SET(4 4) "))
    it = tr.__iter__()
    assert type(code := next(it)) is Error


def test_no_open_2arity():
    tr = Translator(StringIO("SET X,4) "))
    it = tr.__iter__()
    assert type(code := next(it)) is Error


def test_no_close_2arity():
    tr = Translator(StringIO("SET(X,4 "))
    it = tr.__iter__()
    assert type(code := next(it)) is Error


def test_extra_arg_2arity():
    tr = Translator(StringIO("SET(X,4,5)"))
    it = tr.__iter__()
    assert type(code := next(it)) is Error


def test_bad_arg_2arity():
    tr = Translator(StringIO("SET(X,\n)"))
    it = tr.__iter__()
    assert type(code := next(it)) is Error
