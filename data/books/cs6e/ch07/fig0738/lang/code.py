import abc

from .arguments import AArgument
from .mnemonics import Mnemonics


class ACode(abc.ABC):
    @abc.abstractmethod
    def generate_code(self) -> str:
        pass

    @abc.abstractmethod
    def generate_listing(self) -> str:
        pass


class EmptyInstr(ACode):

    def generate_code(self) -> str:
        return ""

    def generate_listing(self) -> str:
        return ""


class UnaryInstr(ACode):
    def __init__(self, mnemonic: Mnemonics):
        self.mnemonic = mnemonic

    def generate_code(self) -> str:
        match self.mnemonic:
            case Mnemonics.STOP:
                return f"{self.mnemonic}"
        return ""

    def generate_listing(self) -> str:
        return f"{self.mnemonic}"


class OneArgInstr(ACode):
    def __init__(self, mnemonic: Mnemonics, arg: AArgument):
        self.mnemonic = mnemonic
        self.arg = arg

    def generate_code(self) -> str:
        arg_code = self.arg.generate_code()
        match self.mnemonic:
            case Mnemonics.ABS:
                return f"{arg_code} <- |{arg_code}|"
            case Mnemonics.NEG:
                return f"{arg_code} <- -{arg_code}"
        return ""

    def generate_listing(self) -> str:
        return f"{self.mnemonic}({self.arg.generate_code()})"


class TwoArgInstr(ACode):
    def __init__(self, mnemonic: Mnemonics, arg1: AArgument, arg2: AArgument):
        self.mnemonic = mnemonic
        self.arg1, self.arg2 = arg1, arg2

    def generate_code(self) -> str:
        arg1_code, arg2_code = self.arg1.generate_code(), self.arg2.generate_code()
        match self.mnemonic:
            case Mnemonics.SET:
                return f"{arg1_code} <- {arg2_code}"
            case Mnemonics.ADD:
                return f"{arg1_code} <- {arg1_code} + {arg2_code}"
            case Mnemonics.SUB:
                return f"{arg1_code} <- {arg1_code} - {arg2_code}"
            case Mnemonics.MUL:
                return f"{arg1_code} <- {arg1_code} * {arg2_code}"
            case Mnemonics.DIV:
                return f"{arg1_code} <- {arg1_code} / {arg2_code}"
        return ""

    def generate_listing(self) -> str:
        return (
            f"{self.mnemonic}({self.arg1.generate_code()}, {self.arg2.generate_code()})"
        )


class Error(ACode):
    def __init__(self, message: str):
        self.message = message

    def generate_code(self) -> str:
        return ""

    def generate_listing(self) -> str:
        return f"ERROR: {self.message}"
