from io import StringIO

import pytest

from pep10.lexer import Lexer, Tokens


def test_lexer_empty():
    tk = Lexer(StringIO("   \n  "))
    assert next(tk) == (Tokens.EMPTY, None)
    assert next(tk) == (Tokens.EMPTY, None)


def test_lexer_comma():
    tk = Lexer(StringIO("   ,\n,  "))
    assert next(tk) == (Tokens.COMMA, None)
    assert next(tk) == (Tokens.EMPTY, None)
    assert next(tk) == (Tokens.COMMA, None)
    assert next(tk) == (Tokens.EMPTY, None)


def test_lexer_comment():
    tk = Lexer(StringIO(" ;Comment here\n"))
    assert next(tk) == (Tokens.COMMENT, "Comment here")
    assert next(tk) == (Tokens.EMPTY, None)


def test_lexer_identifier():
    tk = Lexer(StringIO("a bCd b0 b9 a_word "))
    assert next(tk) == (Tokens.IDENTIFIER, "a")
    assert next(tk) == (Tokens.IDENTIFIER, "bCd")
    assert next(tk) == (Tokens.IDENTIFIER, "b0")
    assert next(tk) == (Tokens.IDENTIFIER, "b9")
    assert next(tk) == (Tokens.IDENTIFIER, "a_word")


def test_lexer_symbol():
    tk = Lexer(StringIO("a: bCd: b0: b9: a_word: "))
    assert next(tk) == (Tokens.SYMBOL, "a")
    assert next(tk) == (Tokens.SYMBOL, "bCd")
    assert next(tk) == (Tokens.SYMBOL, "b0")
    assert next(tk) == (Tokens.SYMBOL, "b9")
    assert next(tk) == (Tokens.SYMBOL, "a_word")


def test_lexer_unsigned_decimal():
    tk = Lexer(StringIO("0 00 000 10 65537 "))
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, 10)
    assert next(tk) == (Tokens.DECIMAL, 65537)


def test_lexer_positive_decimal():
    tk = Lexer(StringIO("+0 +00 +000 +10 +65537 "))
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, 10)
    assert next(tk) == (Tokens.DECIMAL, 65537)


def test_lexer_negative_decimal():
    tk = Lexer(StringIO("-0 -00 -000 -10 -65537 "))
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, 0)
    assert next(tk) == (Tokens.DECIMAL, -10)
    assert next(tk) == (Tokens.DECIMAL, -65537)


def test_lexer_sign_needs_digit():
    tk = Lexer(StringIO("- "))
    assert next(tk) == (Tokens.INVALID, None)


def test_lexer_hexadecimal():
    tk = Lexer(StringIO("0x0 0X000  0x1 0x10 0x10000 "))
    assert next(tk) == (Tokens.HEX, 0)
    assert next(tk) == (Tokens.HEX, 0)
    assert next(tk) == (Tokens.HEX, 1)
    assert next(tk) == (Tokens.HEX, 0x10)
    assert next(tk) == (Tokens.HEX, 0x1_00_00)


def test_lexer_hex_needs_digit():
    tk = Lexer(StringIO("0x "))
    assert next(tk) == (Tokens.INVALID, None)


def test_lexer_dot():
    tk = Lexer(StringIO(".a .bCd .b0 .b9 .a_word "))
    assert next(tk) == (Tokens.DOT, "a")
    assert next(tk) == (Tokens.DOT, "bCd")
    assert next(tk) == (Tokens.DOT, "b0")
    assert next(tk) == (Tokens.DOT, "b9")
    assert next(tk) == (Tokens.DOT, "a_word")


def test_lexer_dot_requires_char():
    tk = Lexer(StringIO(". "))
    assert next(tk) == (Tokens.INVALID, None)

    tk = Lexer(StringIO(".0 "))
    assert next(tk) == (Tokens.INVALID, None)


@pytest.mark.skip("Implement in Problem 7.##")  # FIGURE ONLY
def test_lexer_unescaped_string():
    tk = Lexer(StringIO('"Hello world"'))
    assert next(tk) == (Tokens.STRING, b"Hello world")
    tk = Lexer(StringIO('""'))
    assert next(tk) == (Tokens.STRING, b"")
    tk = Lexer(StringIO('" "'))
    assert next(tk) == (Tokens.STRING, b" ")
    tk = Lexer(StringIO('"\'"'))
    assert next(tk) == (Tokens.STRING, b"'")


@pytest.mark.skip("Implement in Problem 7.##")  # FIGURE ONLY
def test_lexer_nonhex_escape_string():
    tk = Lexer(StringIO('"\\r\\t\\n\\\\\\""'))
    assert next(tk) == (Tokens.STRING, b'\r\t\n\\"')
    # Invalid escape character
    tk = Lexer(StringIO('"\\a"'))
    assert next(tk) == (Tokens.INVALID, None)
