from io import StringIO

from lang.tokenizer import Tokenizer
from lang.tokens import Tokens


def test_tokenizer_empty():
    tk = Tokenizer(StringIO("   \n  "))
    it = tk.__iter__()
    assert next(it) == (Tokens.EMPTY, None)
    assert next(it) == (Tokens.EMPTY, None)


def test_tokenizer_comma():
    tk = Tokenizer(StringIO("   \n  ,  \n , "))
    it = tk.__iter__()
    assert next(it) == (Tokens.EMPTY, None)
    assert next(it) == (Tokens.COMMA, None)
    assert next(it) == (Tokens.EMPTY, None)
    assert next(it) == (Tokens.COMMA, None)


def test_tokenizer_identifier():
    tk = Tokenizer(StringIO(" words are hard "))
    it = tk.__iter__()
    assert next(it) == (Tokens.IDENTIFIER, "words")
    assert next(it) == (Tokens.IDENTIFIER, "are")
    assert next(it) == (Tokens.IDENTIFIER, "hard")


def test_tokenizer_identifier_newlines():
    tk = Tokenizer(StringIO(" words\n are\n hard "))
    it = tk.__iter__()
    assert next(it) == (Tokens.IDENTIFIER, "words")
    assert next(it) == (Tokens.IDENTIFIER.EMPTY, None)
    assert next(it) == (Tokens.IDENTIFIER, "are")
    assert next(it) == (Tokens.IDENTIFIER.EMPTY, None)
    assert next(it) == (Tokens.IDENTIFIER, "hard")


def test_tokenizer_integer():
    tk = Tokenizer(StringIO("-10 10 +10 0 "))
    it = tk.__iter__()
    assert next(it) == (Tokens.INTEGER, -10)
    assert next(it) == (Tokens.INTEGER, 10)
    assert next(it) == (Tokens.INTEGER, 10)
    assert next(it) == (Tokens.INTEGER, 0)


def test_tokenizer_invalid():
    tk = Tokenizer(StringIO("-a ! ; "))
    it = tk.__iter__()
    assert next(it) == (Tokens.INVALID, None)
    assert next(it) == (Tokens.INVALID, None)
    assert next(it) == (Tokens.INVALID, None)
    assert next(it) == (Tokens.EMPTY, None)


def test_tokenizer_left_paren():
    tk = Tokenizer(StringIO("( "))
    it = tk.__iter__()
    assert next(it) == (Tokens.LEFT_PAREN, None)
    assert next(it) == (Tokens.EMPTY, None)


def test_tokenizer_right_paren():
    tk = Tokenizer(StringIO(") "))
    it = tk.__iter__()
    assert next(it) == (Tokens.RIGHT_PAREN, None)
    assert next(it) == (Tokens.EMPTY, None)
