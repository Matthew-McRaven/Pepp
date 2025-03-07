import itertools
from typing import List, TypeAlias, Literal

from pep10.arguments import StringConstant
from pep10.mnemonics import (
    AddressingMode,
    INSTRUCTION_TYPES,
    BITS,
    as_int,
)
from pep10.symbol import SymbolEntry
from pep10.types import ArgumentType, Listable, ParseTreeNode


def source(
    op: str,
    args: List[str],
    symbol: SymbolEntry | None = None,
    comment: str | None = None,
) -> str:
    sym_str = f"{symbol}:" if symbol else ""
    comment_str = f";{comment}" if comment else ""
    return f"{sym_str:7}{op:7}{','.join(args):12}{comment_str}"


class ErrorNode:
    def __init__(self, error: str | None = None) -> None:
        self.comment: str | None = error

    def source(self) -> str:
        message = self.comment or "Failed to parse line"
        return f";ERROR: {message}"


class EmptyNode:
    def __init__(self) -> None:
        self.comment: str | None = None

    def source(self) -> str:
        return source("", [], None, None)


class CommentNode:
    def __init__(self, comment: str):
        self.comment: str | None = comment

    def source(self) -> str:
        return source("", [], None, self.comment)


class UnaryNode:
    def __init__(
        self,
        mn: str,
        sym: SymbolEntry | None = None,
        comment: str | None = None,
    ):
        self.symbol_decl: SymbolEntry | None = sym
        mn = mn.upper()
        assert mn in INSTRUCTION_TYPES
        self.mnemonic = mn
        self.comment: str | None = comment

    def source(self) -> str:
        return source(
            str(self.mnemonic.upper()),
            [],
            self.symbol_decl,
            self.comment,
        )


class NonUnaryNode:
    def __init__(
        self,
        mn: str,
        argument: ArgumentType,
        am: AddressingMode,
        sym: SymbolEntry | None = None,
        comment: str | None = None,
    ):
        self.symbol_decl: SymbolEntry | None = sym
        mn = mn.upper()
        assert mn in INSTRUCTION_TYPES
        self.mnemonic = mn
        self.addressing_mode: AddressingMode = am
        self.argument: ArgumentType = argument
        self.comment: str | None = comment

    def source(self) -> str:
        args = [str(self.argument), self.addressing_mode.name.lower()]
        return source(
            self.mnemonic.upper(), args, self.symbol_decl, self.comment
        )


class DotASCIINode:
    def __init__(
        self,
        argument: StringConstant,
        sym: SymbolEntry | None = None,
        comment: str | None = None,
    ):
        self.symbol_decl: SymbolEntry | None = sym
        self.argument: StringConstant = argument
        self.comment: str | None = comment

    def source(self) -> str:
        args = [str(self.argument)]
        return source(".ASCII", args, self.symbol_decl, self.comment)


class DotLiteralNode:
    def __init__(
        self,
        argument: ArgumentType,
        width: Literal[1, 2] = 1,
        sym: SymbolEntry | None = None,
        comment: str | None = None,
    ):
        self.symbol_decl: SymbolEntry | None = sym
        self.argument: ArgumentType = argument
        self.width: int = width
        self.comment: str | None = comment

    def source(self) -> str:
        args = [str(self.argument)]
        name = ".BYTE" if self.width == 1 else ".WORD"
        return source(name, args, self.symbol_decl, self.comment)


class DotBlockNode:
    def __init__(
        self,
        argument: ArgumentType,
        sym: SymbolEntry | None = None,
        comment: str | None = None,
    ):
        self.symbol_decl: SymbolEntry | None = sym
        self.argument = argument
        self.comment: str | None = comment

    def source(self) -> str:
        return source(
            ".BLOCK",
            [str(self.argument)],
            self.symbol_decl,
            self.comment,
        )


class DotEquateNode:
    def __init__(
        self,
        argument: ArgumentType,
        sym: SymbolEntry | None = None,
        comment: str | None = None,
    ):
        self.symbol_decl: SymbolEntry | None = sym
        self.argument: ArgumentType = argument
        self.comment: str | None = comment

    def source(self) -> str:
        args = [str(self.argument)]
        return source(".EQUATE", args, self.symbol_decl, self.comment)


class MacroNode:
    def __init__(
        self,
        name: str,
        args: List[ArgumentType],
        body: List[ParseTreeNode],
        comment: str | None = None,
    ):
        self.symbol_decl: SymbolEntry | None = None
        self.name = name
        self.arguments = args
        self.body = body
        self.comment = comment

    def source(self) -> str:
        args = [str(arg) for arg in self.arguments]
        return source(f"@{self.name}", args, None, self.comment)


def listing(to_list: Listable) -> List[str]:
    object_code = to_list.object_code()
    oc_format = lambda oc: "".join(f"{i:02X}" for i in oc)
    if len(object_code) <= 3:
        line_object_code, object_code = object_code, bytearray([])
    else:
        line_object_code, object_code = (
            object_code[0:2],
            object_code[3:],
        )
    address = (
        f"{to_list.address:04X}"
        if to_list.address is not None
        else 4 * " "
    )
    lines = [
        f"{address} {oc_format(line_object_code):6} {to_list.source()}"
    ]
    for b in itertools.batched(object_code, 3):
        lines.append(f"{'':4} {oc_format(b): 6}")
    return lines


class EmptyIR(EmptyNode):
    def __init__(self) -> None:
        super().__init__()
        self.address: int | None = None

    def object_code(self) -> bytearray:
        return bytearray()

    def __len__(self):
        return 0


class CommentIR(CommentNode):
    def __init__(self, comment: str) -> None:
        super().__init__(comment)
        self.address: int | None = None

    def object_code(self) -> bytearray:
        return bytearray()

    def __len__(self):
        return 0


class UnaryIR(UnaryNode):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.address: int | None = None

    def object_code(self) -> bytearray:
        bits = BITS[self.mnemonic]
        return bytearray(bits.to_bytes(1))

    def __len__(self) -> int:
        return 1


class NonUnaryIR(NonUnaryNode):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.address: int | None = None

    def object_code(self) -> bytearray:
        bits = as_int(self.mnemonic, am=self.addressing_mode)
        mn_bytes = bits.to_bytes(1, signed=False)
        arg_bytes = int(self.argument).to_bytes(2)
        return bytearray(mn_bytes + arg_bytes)

    def __len__(self) -> int:
        return 3


class DotASCIIIR(DotASCIINode):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.address: int | None = None

    def object_code(self) -> bytearray:
        return bytearray(self.argument.value)

    def __len__(self) -> int:
        return len(self.argument.value)


class DotLiteralIR(DotLiteralNode):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.address: int | None = None

    def object_code(self) -> bytearray:
        return bytearray(int(self.argument) * [0])

    def __len__(self) -> int:
        return int(self.width)


class DotBlockIR(DotBlockNode):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.address: int | None = None

    def object_code(self) -> bytearray:
        return bytearray(int(self.argument) * [0])

    def __len__(self) -> int:
        return int(self.argument)


class DotEquateIR(DotEquateNode):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.address: int | None = None

    def object_code(self) -> bytearray:
        return bytearray()

    def __len__(self) -> int:
        return 0


class MacroIR(MacroNode):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.address: int | None = None

    def object_code(self) -> bytearray:
        ret = bytearray()
        for line in self.body:
            if isinstance(line, Listable):
                ret.extend(line.object_code())
        return ret

    def __len__(self) -> int:
        ret = 0
        for line in self.body:
            if isinstance(line, Listable):
                ret += len(line)
        return ret

    def start_comment(self) -> CommentIR:
        return CommentIR(self.source().lstrip())

    def end_comment(self) -> CommentIR:
        return CommentIR(f"End @{self.name}")


DotCommandIR: TypeAlias = (
    DotASCIIIR | DotLiteralIR | DotBlockIR | DotEquateIR
)
