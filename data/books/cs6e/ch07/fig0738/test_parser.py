import io
from typing import cast

import pytest

from pep10.arguments import (
    Decimal,
    Hexadecimal,
    Identifier,
    StringConstant,
)
from pep10.ir import (
    UnaryIR,
    NonUnaryIR,
    CommentNode,
    EmptyNode,
    ErrorNode,
    CommentIR,
    EmptyIR,
    DotASCIIIR,
    DotBlockIR,
    DotLiteralIR,
    DotEquateIR,
    MacroNode,
    MacroIR,
)
from pep10.macro import MacroRegistry
from pep10.mnemonics import AddressingMode
from pep10.parser import Parser, parse


def test_unary_pass() -> None:
    par = Parser(io.StringIO("RET \n"))
    item: UnaryIR = cast(UnaryIR, next(par))
    assert type(item) is UnaryIR
    assert item.mnemonic == "RET"

    res = parse("caT:NOTA \n")
    item = cast(UnaryIR, res[0])
    assert type(item) is UnaryIR
    assert item.mnemonic == "NOTA"
    assert str(item.symbol_decl) == "caT"


def test_unary_fail() -> None:
    res = parse("RETS \n")
    assert type(res[0]) is ErrorNode

    res = parse("RETS \n")
    assert type(res[0]) is ErrorNode


@pytest.mark.skip("Implement in Problem 7.##")  # FIGURE ONLY
def test_macro() -> None:
    mr = MacroRegistry()
    mr.insert("HI", 2, ";WORLD")
    res = parse("@HI 2, 4\n", macro_registry=mr)
    assert type(res[0]) is MacroIR
    item: MacroNode = res[0]
    assert len(item.body) == 1
    assert type(item.body[0]) is CommentIR


def test_nonunary() -> None:
    par = Parser(io.StringIO("BR 10,i \n"))
    item: NonUnaryIR = cast(NonUnaryIR, next(par))
    assert type(item) is NonUnaryIR
    assert item.mnemonic == "BR"
    assert type(item.argument) is Decimal

    ret = parse("cat: BR 0x10,x ;comment\n")
    item = cast(NonUnaryIR, ret[0])
    assert type(item) is NonUnaryIR
    assert str(item.symbol_decl) == "cat"
    assert item.mnemonic == "BR"
    assert type(item.argument) is Hexadecimal
    assert item.comment == "comment"

    ret = parse("cat: BR cat,i")
    item = cast(NonUnaryIR, ret[0])
    assert type(item) is NonUnaryIR
    assert str(item.symbol_decl) == "cat"
    assert item.mnemonic == "BR"
    assert type(item.argument) is Identifier
    arg: Identifier = item.argument
    assert str(arg) == "cat"
    assert not arg.symbol.is_undefined()
    assert not arg.symbol.is_multiply_defined()


@pytest.mark.skip("Implement in Problem 7.##")  # FIGURE ONLY
def test_nonunary_with_strings() -> None:
    ret = parse('cat: BR "h\'",i')
    item = cast(NonUnaryIR, ret[0])
    assert type(item) is NonUnaryIR
    assert str(item.symbol_decl) == "cat"
    assert item.mnemonic == "BR"
    assert type(item.argument) is StringConstant
    arg_str = item.argument
    assert int(arg_str).to_bytes(2) == "h'".encode("utf-8")
    assert str(arg_str) == '"h\'"'

    ret = parse('cat: BR "\\r\\"",i')
    item = cast(NonUnaryIR, ret[0])
    assert type(item) is NonUnaryIR
    assert str(item.symbol_decl) == "cat"
    assert item.mnemonic == "BR"
    assert type(item.argument) is StringConstant
    arg_str = item.argument
    assert int(arg_str).to_bytes(2) == '\r"'.encode("utf-8")
    assert str(arg_str) == '"\\r\\""'


def test_nonunary_fail() -> None:
    par = Parser(io.StringIO("ADDA 10\n"))
    assert type(next(par)) is ErrorNode

    ret = parse("ADDA 10 ,\n")
    assert type(ret[0]) is ErrorNode

    ret = parse("ADDA 10,cat\n")
    assert type(ret[0]) is ErrorNode

    ret = parse("ADDA cat:,sfx\n")
    assert type(ret[0]) is ErrorNode


@pytest.mark.skip("Implement in Problem 7.##")  # FIGURE ONLY
def test_nonunary_addr_optional() -> None:
    ret = parse("BR 10\n")
    item: NonUnaryIR = cast(NonUnaryIR, ret[0])
    assert type(item) is NonUnaryIR
    assert item.mnemonic == "BR"
    assert type(item.argument) is Decimal
    assert int(item.argument) == 10
    assert item.addressing_mode == AddressingMode.I


def test_nonunary_arg_range() -> None:
    ret = parse("BR 65535, i\n")
    assert not type(ret[0]) is ErrorNode
    ret = parse("BR 65536, i\n")
    assert type(ret[0]) is ErrorNode
    ret = parse("BR -32768, i\n")
    assert not type(ret[0]) is ErrorNode
    ret = parse("BR -32769, i\n")
    assert type(ret[0]) is ErrorNode
    ret = parse("BR 0xFFFF, i\n")
    assert not type(ret[0]) is ErrorNode
    ret = parse("BR 0x10000, i\n")
    assert type(ret[0]) is ErrorNode


def test_comment() -> None:
    par = Parser(io.StringIO("  ;comment \n"))
    item: CommentNode = cast(CommentNode, next(par))
    assert type(item) is CommentIR
    assert item.comment == "comment "


def test_empty() -> None:
    par = Parser(io.StringIO("\n"))
    item: EmptyNode = cast(EmptyNode, next(par))
    assert type(item) is EmptyIR


def test_parser_synchronization() -> None:
    ret = parse("NOPN HELLO CRUEL: WORLD\nNOPN\nRET\n")
    assert len(ret) == 3


@pytest.mark.skip("Implement in Problem 7.##")  # FIGURE ONLY
def test_dot_ASCII() -> None:
    ret = parse('cat: .ASCII "Hello World"')
    assert len(ret) == 1
    item: DotASCIIIR = cast(DotASCIIIR, ret[0])
    assert type(item) is DotASCIIIR
    assert str(item.argument) == '"Hello World"'
    assert item.symbol_decl and str(item.symbol_decl) == "cat"
    ret = parse('.ASCII ""')
    item = cast(DotASCIIIR, ret[0])
    assert type(item) is DotASCIIIR
    assert str(item.argument) == '""'


def test_dot_block() -> None:
    ret = parse("cat: .BLOCK 0x10")
    assert len(ret) == 1
    item: DotBlockIR = cast(DotBlockIR, ret[0])
    assert type(item) is DotBlockIR
    assert int(item.argument) == 0x10
    assert len(item) == 0x10
    assert item.symbol_decl and str(item.symbol_decl) == "cat"
    ret = parse(".BLOCK 10")
    item = cast(DotBlockIR, ret[0])
    assert type(item) is DotBlockIR
    assert int(item.argument) == 10
    assert len(item) == 10


def test_dot_byte() -> None:
    ret = parse("cat: .BYte 0x10")
    assert len(ret) == 1
    item: DotLiteralIR = cast(DotLiteralIR, ret[0])
    assert type(item) is DotLiteralIR
    assert int(item.argument) == 0x10
    assert len(item) == 1
    assert item.symbol_decl and str(item.symbol_decl) == "cat"
    ret = parse(".BYte 10")
    item = cast(DotLiteralIR, ret[0])
    assert type(item) is DotLiteralIR
    assert int(item.argument) == 10
    assert len(item) == 1


@pytest.mark.skip("Implement in Problem 7.##")  # FIGURE ONLY
def test_dot_equate() -> None:
    ret = parse("cat: .EQUATE 0x10")
    assert len(ret) == 1
    item: DotEquateIR = cast(DotEquateIR, ret[0])
    assert type(item) is DotEquateIR
    assert int(item.argument) == 0x10
    assert len(item) == 0
    assert item.symbol_decl and str(item.symbol_decl) == "cat"
    assert int(item.symbol_decl) == 0x10

    ret = parse("cat: .EQUATE 10")
    assert len(ret) == 1
    item = cast(DotEquateIR, ret[0])
    assert type(item) is DotEquateIR
    assert int(item.argument) == 10
    assert len(item) == 0
    assert item.symbol_decl and str(item.symbol_decl) == "cat"
    assert int(item.symbol_decl) == 10

    # Strings
    ret = parse('cat: .EQUATE "\\x0a"\n')
    assert len(ret) == 1
    item = cast(DotEquateIR, ret[0])
    assert type(item) is DotEquateIR
    assert int(item.argument) == 10
    assert len(item) == 0
    assert item.symbol_decl and str(item.symbol_decl) == "cat"
    assert int(item.symbol_decl) == 10

    # Symbolic argument
    ret = parse("cat: .EQUATE cat")
    item = cast(DotEquateIR, ret[0])
    assert type(item) is ErrorNode


def test_dot_word() -> None:
    ret = parse("cat: .WORd 0x10")
    assert len(ret) == 1
    item: DotLiteralIR = cast(DotLiteralIR, ret[0])
    assert type(item) is DotLiteralIR
    assert int(item.argument) == 0x10
    assert len(item) == 2
    assert item.symbol_decl and str(item.symbol_decl) == "cat"
    ret = parse(".woRD 10")
    item = cast(DotLiteralIR, ret[0])
    assert type(item) is DotLiteralIR
    assert int(item.argument) == 10
    assert len(item) == 2
