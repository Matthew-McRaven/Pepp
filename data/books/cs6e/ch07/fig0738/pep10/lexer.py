import io
import os
import string
from enum import Enum
from typing import Literal, List

from pep10.tokens import Tokens, TokenType


class Lexer:
    class States(Enum):

        START, COMMENT, IDENTIFIER, MAYBE_HEX, HEX_PREFIX = range(0, 5)
        HEX, MAYBE_SIGNED, DECIMAL, MAYBE_DOT, DOT = range(5, 10)
        CHAR_OPEN, CHAR_AWAITING_CLOSE = 10, 11
        CHAR_EXPECT_ESCAPE, CHAR_EXPECT_HEX0 = 12, 13
        CHAR_EXPECT_HEX1, STRING_AWAITING_CLOSE = 14, 15
        STRING_EXPECT_ESCAPE, STRING_EXPECT_HEX0 = 16, 17
        STRING_EXPECT_HEX1, MAYBE_MACRO, MACRO, STOP = range(18, 22)

    def __init__(self, buffer) -> None:
        self.buffer: io.StringIO = buffer

    def __iter__(self) -> "Lexer":
        return self

    def __next__(self) -> "TokenType":
        prev_pos = self.buffer.tell()
        next_ch = self.buffer.read(1)
        if len(next_ch) == 0:
            raise StopIteration
        else:
            self.buffer.seek(prev_pos, os.SEEK_SET)

        as_str_list: List[str] = []
        as_bytes: bytes = bytes()
        as_int: int = 0
        sign: Literal[-1, 1] = 1
        state: Lexer.States = Lexer.States.START
        token: "TokenType" = (Tokens.EMPTY, None)

        while state != Lexer.States.STOP and token[0] != Tokens.INVALID:
            prev_pos = self.buffer.tell()
            ch: str = self.buffer.read(1)
            if len(ch) == 0:
                break
            match state:
                case Lexer.States.START:
                    if ch == "\n":
                        state = Lexer.States.STOP
                    elif ch == ",":
                        state = Lexer.States.STOP
                        token = (Tokens.COMMA, None)
                    elif ch.isspace():
                        pass
                    elif ch == ";":
                        state = Lexer.States.COMMENT
                    elif ch.isalpha():
                        as_str_list.append(ch)
                        state = Lexer.States.IDENTIFIER
                    elif ch == "0":
                        state = Lexer.States.MAYBE_HEX
                    elif ch.isdecimal():
                        state = Lexer.States.DECIMAL
                        as_int = ord(ch) - ord("0")
                    elif ch == ".":
                        state = Lexer.States.MAYBE_DOT
                    elif ch == "'":
                        state = Lexer.States.CHAR_OPEN
                    elif ch == '"':
                        state = Lexer.States.STRING_AWAITING_CLOSE
                    elif ch == "+" or ch == "-":
                        state = Lexer.States.MAYBE_SIGNED
                        sign = -1 if ch == "-" else 1
                    elif ch == "@":
                        state = Lexer.States.MAYBE_MACRO
                    else:
                        token = (Tokens.INVALID, None)

                case Lexer.States.COMMENT:
                    if ch == "\n":
                        self.buffer.seek(prev_pos, os.SEEK_SET)
                        state = Lexer.States.STOP
                        token = (Tokens.COMMENT, "".join(as_str_list))
                    else:
                        as_str_list.append(ch)

                case Lexer.States.IDENTIFIER:
                    if ch.isalnum() or ch == "_":
                        as_str_list.append(ch)
                    elif ch == ":":
                        state = Lexer.States.STOP
                        token = (Tokens.SYMBOL, "".join(as_str_list))
                    else:
                        self.buffer.seek(prev_pos, os.SEEK_SET)
                        state = Lexer.States.STOP
                        as_str = "".join(as_str_list)
                        token = (Tokens.IDENTIFIER, as_str)

                case Lexer.States.MAYBE_HEX:
                    if ch.isdigit():
                        as_int = ord(ch) - ord("0")
                        state = Lexer.States.DECIMAL
                    elif ch == "x" or ch == "X":
                        state = Lexer.States.HEX_PREFIX
                    else:
                        self.buffer.seek(prev_pos, os.SEEK_SET)
                        state = Lexer.States.STOP
                        token = (Tokens.DECIMAL, 0)

                case Lexer.States.HEX_PREFIX:
                    if ch in string.digits:
                        state = Lexer.States.HEX
                        as_int = as_int * 16 + (ord(ch) - ord("0"))
                    elif ch in string.hexdigits:
                        state = Lexer.States.HEX
                        digit = 10 + ord(ch.lower()) - ord("a")
                        as_int = as_int * 16 + digit
                    else:
                        token = (Tokens.INVALID, None)

                case Lexer.States.HEX:
                    if ch in string.digits:
                        as_int = as_int * 16 + (ord(ch) - ord("0"))
                    elif ch in string.hexdigits:
                        digit = 10 + ord(ch.lower()) - ord("a")
                        as_int = as_int * 16 + digit
                    else:
                        self.buffer.seek(prev_pos, os.SEEK_SET)
                        state = Lexer.States.STOP
                        token = (Tokens.HEX, as_int)

                case Lexer.States.MAYBE_SIGNED:
                    if ch in string.digits:
                        as_int = as_int * 10 + (ord(ch) - ord("0"))
                        state = Lexer.States.DECIMAL
                    else:
                        token = (Tokens.INVALID, None)

                case Lexer.States.DECIMAL:
                    if ch in string.digits:
                        as_int = as_int * 10 + (ord(ch) - ord("0"))
                    else:
                        self.buffer.seek(prev_pos, os.SEEK_SET)
                        state = Lexer.States.STOP
                        token = (Tokens.DECIMAL, sign * as_int)

                case Lexer.States.MAYBE_DOT:
                    if ch.isalpha():
                        as_str_list.append(ch)
                        state = Lexer.States.DOT
                    else:
                        token = (Tokens.INVALID, None)

                case Lexer.States.DOT:
                    if ch.isalnum() or ch == "_":
                        as_str_list.append(ch)
                    else:
                        self.buffer.seek(prev_pos, os.SEEK_SET)
                        state = Lexer.States.STOP
                        token = (Tokens.DOT, "".join(as_str_list))

                case Lexer.States.MAYBE_MACRO:
                    if ch.isalpha():
                        as_str_list.append(ch)
                        state = Lexer.States.MACRO
                    else:
                        token = (Tokens.INVALID, None)

                case Lexer.States.MACRO:
                    if ch.isalnum() or ch == "_":
                        as_str_list.append(ch)
                    else:
                        self.buffer.seek(prev_pos, os.SEEK_SET)
                        state = Lexer.States.STOP
                        token = (Tokens.MACRO, "".join(as_str_list))

                case Lexer.States.STRING_AWAITING_CLOSE:
                    if ch == '"':
                        token = (Tokens.STRING, as_bytes)
                        state = Lexer.States.STOP
                    elif ch == "\\":
                        state = Lexer.States.STRING_EXPECT_ESCAPE
                    else:
                        as_bytes = as_bytes + ch.encode("utf-8")

                case Lexer.States.STRING_EXPECT_ESCAPE:
                    escapes = dict(zip('rtbn"\\', '\r\t\b\n"\\'))
                    if ch in escapes:
                        state = Lexer.States.STRING_AWAITING_CLOSE
                        as_bytes += escapes[ch].encode("utf-8")
                    elif ch in "xX":
                        state = Lexer.States.STRING_EXPECT_HEX0
                    else:
                        token = (Tokens.INVALID, None)

                case Lexer.States.STRING_EXPECT_HEX0:
                    if ch in string.digits:
                        state = Lexer.States.STRING_EXPECT_HEX1
                        as_int = as_int * 16 + (ord(ch) - ord("0"))
                    elif ch in string.hexdigits:
                        state = Lexer.States.STRING_EXPECT_HEX1
                        digit = 10 + ord(ch.lower()) - ord("a")
                        as_int = as_int * 16 + digit

                case Lexer.States.STRING_EXPECT_HEX1:
                    if ch in string.digits:
                        state = Lexer.States.STRING_AWAITING_CLOSE
                        as_int = as_int * 16 + (ord(ch) - ord("0"))
                        as_bytes += as_int.to_bytes(1)
                    elif ch in string.hexdigits:
                        state = Lexer.States.STRING_AWAITING_CLOSE
                        digit = 10 + ord(ch.lower()) - ord("a")
                        as_int = as_int * 16 + digit
                        as_bytes += as_int.to_bytes(1)
                    elif ch == '"':
                        state = Lexer.States.STOP
                        as_bytes += as_int.to_bytes(1)
                        token = (Tokens.STRING, as_bytes)

                case _:
                    token = (Tokens.INVALID, None)

        return token

    def skip_to_next_line(self):
        while (ch := self.buffer.read(1)) != "\n" and len(ch) > 0:
            pass
