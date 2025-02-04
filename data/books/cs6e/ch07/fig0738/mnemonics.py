from enum import Enum


class Mnemonic:
    def __init__(self, is_unary: bool | None):
        self.is_unary = is_unary


class Mnemonics(Enum):
    ADD = Mnemonic(False)
    SUB = Mnemonic(False)
    MUL = Mnemonic(False)
    DIV = Mnemonic(False)
    NEG = Mnemonic(True)
    ABS = Mnemonic(True)
    SET = Mnemonic(False)
    STOP = Mnemonic(None)
    END = Mnemonic(None)

    def __str__(self):
        return f"{self.name.lower()}"

    def is_unary(self):
        return self.value.is_unary == True

    def is_nonunary(self):
        return self.value.is_unary == False
