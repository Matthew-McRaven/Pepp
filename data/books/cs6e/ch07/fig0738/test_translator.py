from io import StringIO
from typing import cast

from lang.arguments import IdentifierArgument, IntegerArgument
from lang.code import UnaryInstr, Error, OneArgInstr, TwoArgInstr
from lang.mnemonics import Mnemonics
from lang.translator import Translator


def test_0arity():
    tr = Translator(StringIO("STOP\n  END "))
    assert type(code := tr.parse_line()) is UnaryInstr
    assert cast(UnaryInstr, code).mnemonic == Mnemonics.STOP

    assert type(code := tr.parse_line()) is UnaryInstr
    assert cast(UnaryInstr, code).mnemonic == Mnemonics.END


def test_fake_0arity():
    tr = Translator(StringIO("STOPS\n"))
    assert type(code := tr.parse_line()) is Error


def test_1arity():
    tr = Translator(StringIO("ABS(X)\n  NEG(F) "))
    assert type(code := tr.parse_line()) is OneArgInstr
    assert cast(OneArgInstr, code).mnemonic == Mnemonics.ABS
    assert type(arg := cast(OneArgInstr, code).arg) == IdentifierArgument
    assert arg.generate_code() == "X"

    assert type(code := tr.parse_line()) is OneArgInstr
    assert cast(OneArgInstr, code).mnemonic == Mnemonics.NEG
    assert type(arg := cast(OneArgInstr, code).arg) == IdentifierArgument
    assert arg.generate_code() == "F"


def test_fake_1arity():
    tr = Translator(StringIO("ABSED(X) "))
    assert type(code := tr.parse_line()) is Error


def test_no_close_1arity():
    tr = Translator(StringIO("ABS(X "))
    assert type(code := tr.parse_line()) is Error


def test_2arity():
    tr = Translator(StringIO("SET(X,4)\nADD(Y, 2)\nMUL(X,X)\nDIV(X,-1)\nSUB(X,Y) "))
    assert type(code := tr.parse_line()) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.SET
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "X"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IntegerArgument
    assert arg.generate_code() == "4"

    assert type(code := tr.parse_line()) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.ADD
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "Y"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IntegerArgument
    assert arg.generate_code() == "2"

    assert type(code := tr.parse_line()) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.MUL
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "X"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IdentifierArgument
    assert arg.generate_code() == "X"

    assert type(code := tr.parse_line()) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.DIV
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "X"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IntegerArgument
    assert arg.generate_code() == "-1"

    assert type(code := tr.parse_line()) is TwoArgInstr
    assert cast(TwoArgInstr, code).mnemonic == Mnemonics.SUB
    assert type(arg := cast(TwoArgInstr, code).arg1) == IdentifierArgument
    assert arg.generate_code() == "X"
    assert type(arg := cast(TwoArgInstr, code).arg2) == IdentifierArgument
    assert arg.generate_code() == "Y"


def test_noident_2arity():
    tr = Translator(StringIO("SET(4,4) "))
    assert type(code := tr.parse_line()) is Error


def test_missing_comma_2arity():
    tr = Translator(StringIO("SET(4 4) "))
    assert type(code := tr.parse_line()) is Error


def test_no_open_2arity():
    tr = Translator(StringIO("SET X,4) "))
    assert type(code := tr.parse_line()) is Error


def test_no_close_2arity():
    tr = Translator(StringIO("SET(X,4 "))
    assert type(code := tr.parse_line()) is Error


def test_extra_arg_2arity():
    tr = Translator(StringIO("SET(X,4,5)"))
    assert type(code := tr.parse_line()) is Error


def test_bad_arg_2arity():
    tr = Translator(StringIO("SET(X,\n)"))
    assert type(code := tr.parse_line()) is Error
