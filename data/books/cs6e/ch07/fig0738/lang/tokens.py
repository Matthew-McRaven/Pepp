from enum import Enum


class Tokens(Enum):
    COMMA = 1
    EMPTY = 2
    IDENTIFIER = 3
    INTEGER = 4
    INVALID = 5
    LEFT_PAREN = 6
    RIGHT_PAREN = 7
