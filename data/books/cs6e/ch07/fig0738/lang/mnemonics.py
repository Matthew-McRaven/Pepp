from enum import Enum


class Mnemonic:
    def __init__(self, is_unary: bool):
        self.is_unary = is_unary


class Mnemonics(Enum):
    ADD = Mnemonic(False)
    SUB = Mnemonic(False)
    MUL = Mnemonic(False)
    DIV = Mnemonic(False)
    NEG = Mnemonic(False)
    ABS = Mnemonic(False)
    SET = Mnemonic(False)
    STOP = Mnemonic(True)
    END = Mnemonic(True)

    def __str__(self):
        return f"{self.name.lower()}"

    def is_unary(self):
        return self.value.is_unary

    def is_nonunary(self):
        return not self.value.is_unary
