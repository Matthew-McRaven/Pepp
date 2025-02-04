import os
from enum import Enum
from io import StringIO
from typing import Tuple, Literal, Union, List

from .tokens import Tokens


class Tokenizer:
    class States(Enum):
        START = 1
        IDENT = 2
        SIGN = 3
        INTEGER = 4
        STOP = 5

    def __init__(self, buffer: StringIO):
        self.buffer = buffer
        self.literals_to_tokens = {
            ",": Tokens.COMMA,
            "(": Tokens.LEFT_PAREN,
            ")": Tokens.RIGHT_PAREN,
            "\n": Tokens.EMPTY,
            " ": Tokens.EMPTY,
        }

    def next_token(
        self,
    ) -> Union[
        Tuple[Literal[Tokens.INTEGER], int]
        | Tuple[Literal[Tokens.IDENTIFIER], str]
        | Tuple[Tokens, None]
    ]:

        as_str_list: List[str] = []
        as_int: int = 0
        sign: Literal[-1, 1] = 1
        state: Tokenizer.States = Tokenizer.States.START
        token: Union[
            Tuple[Literal[Tokens.INTEGER], int]
            | Tuple[Literal[Tokens.IDENTIFIER], str]
            | Tuple[Tokens, None]
        ] = (Tokens.EMPTY, None)

        while state != Tokenizer.States.STOP and token[0] != Tokens.INVALID:
            prev_pos = self.buffer.tell()
            ch: str = self.buffer.read(1)
            match state:
                case _ if len(ch) == 0:
                    state = Tokenizer.States.STOP
                case Tokenizer.States.START if ch == "\n":
                    state = Tokenizer.States.STOP
                case Tokenizer.States.START if ch.isspace():
                    pass
                case Tokenizer.States.START if ch.isalpha():
                    as_str_list.append(ch)
                    state = Tokenizer.States.IDENT
                case Tokenizer.States.START if ch == "-" or ch == "+":
                    sign = -1 if ch == "-" else 1
                    state = Tokenizer.States.SIGN
                case Tokenizer.States.START if ch.isdigit():
                    as_int = ord(ch) - ord("0")
                    state = Tokenizer.States.INTEGER
                case Tokenizer.States.START if ch in self.literals_to_tokens:
                    token = (self.literals_to_tokens[ch], None)
                    state = Tokenizer.States.STOP
                case Tokenizer.States.IDENT if ch.isalnum():
                    as_str_list.append(ch)
                case Tokenizer.States.IDENT:
                    token = (Tokens.IDENTIFIER, "".join(as_str_list))
                    self.buffer.seek(prev_pos, os.SEEK_SET)
                    state = Tokenizer.States.STOP
                case Tokenizer.States.SIGN if ch.isdigit():
                    as_int = ord(ch) - ord("0")
                    state = Tokenizer.States.INTEGER
                case Tokenizer.States.INTEGER if ch.isdigit():
                    as_int = 10 * as_int + (ord(ch) - ord("0"))
                case Tokenizer.States.INTEGER:
                    token = (Tokens.INTEGER, sign * as_int)
                    self.buffer.seek(prev_pos, os.SEEK_SET)
                    state = Tokenizer.States.STOP
                case _:
                    token = (Tokens.INVALID, None)
        return token
