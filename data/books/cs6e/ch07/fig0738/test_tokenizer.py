from io import StringIO

from .tokenizer import Tokenizer
from .tokens import Tokens


def test_tokenizer_empty():
    tk = Tokenizer(StringIO("   \n  "))
    assert tk.next_token() == (Tokens.EMPTY, None)
    assert tk.next_token() == (Tokens.EMPTY, None)


def test_tokenizer_comma():
    tk = Tokenizer(StringIO("   \n  ,  \n , "))
    # TODO: I think this is a bug, and should not be commented out
    assert tk.next_token() == (Tokens.EMPTY, None)
    assert tk.next_token() == (Tokens.COMMA, None)
    assert tk.next_token() == (Tokens.EMPTY, None)
    assert tk.next_token() == (Tokens.COMMA, None)


def test_tokenizer_identifier():
    tk = Tokenizer(StringIO(" words are hard "))
    assert tk.next_token() == (Tokens.IDENTIFIER, "words")
    assert tk.next_token() == (Tokens.IDENTIFIER, "are")
    assert tk.next_token() == (Tokens.IDENTIFIER, "hard")


def test_tokenizer_integer():
    tk = Tokenizer(StringIO("-10 10 +10 -0 +0 "))
    assert tk.next_token() == (Tokens.INTEGER, -10)
    assert tk.next_token() == (Tokens.INTEGER, 10)
    assert tk.next_token() == (Tokens.INTEGER, 10)
    assert tk.next_token() == (Tokens.INTEGER, 0)
    assert tk.next_token() == (Tokens.INTEGER, 0)


def test_tokenizer_invalid():
    tk = Tokenizer(StringIO("-a ! ; "))
    assert tk.next_token() == (Tokens.INVALID, None)
    assert tk.next_token() == (Tokens.INVALID, None)
    assert tk.next_token() == (Tokens.INVALID, None)
    assert tk.next_token() == (Tokens.EMPTY, None)


def test_tokenizer_left_paren():
    tk = Tokenizer(StringIO("( "))
    assert tk.next_token() == (Tokens.LEFT_PAREN, None)
    assert tk.next_token() == (Tokens.EMPTY, None)


def test_tokenizer_right_paren():
    tk = Tokenizer(StringIO(") "))
    assert tk.next_token() == (Tokens.RIGHT_PAREN, None)
    assert tk.next_token() == (Tokens.EMPTY, None)
