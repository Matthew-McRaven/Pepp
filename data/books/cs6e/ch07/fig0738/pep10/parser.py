import io
from collections import deque
from typing import Union, cast, List

from pep10.arguments import (
    Hexadecimal,
    Decimal,
    Identifier,
    StringConstant,
)
from pep10.ir import (
    UnaryNode,
    NonUnaryIR,
    UnaryIR,
    ErrorNode,
    NonUnaryNode,
    CommentIR,
    EmptyIR,
    DotLiteralIR,
    DotASCIIIR,
    DotBlockIR,
    DotCommandIR,
    DotEquateIR,
    MacroNode,
    MacroIR,
)
from pep10.lexer import Lexer, Tokens
from pep10.macro import MacroRegistry
from pep10.mnemonics import (
    INSTRUCTION_TYPES,
    InstructionType,
    AddressingMode,
    DEFAULT_ADDRESSING_MODES,
)
from pep10.symbol import SymbolTable, SymbolEntry
from pep10.tokens import TokenType
from pep10.types import ArgumentType, ParseTreeNode


class Parser:
    def __init__(
        self,
        buffer: io.StringIO,
        symbol_table: SymbolTable | None = None,
        macro_registry: MacroRegistry | None = None,
    ):
        self.lexer = Lexer(buffer)
        self._buffer: deque[TokenType] = deque()
        self.symbol_table = (
            symbol_table if symbol_table else SymbolTable()
        )
        self.macro_registry = (
            macro_registry if macro_registry else MacroRegistry()
        )

    def __iter__(self):
        return self

    def may_match(self, expected: Tokens) -> Union[TokenType, None]:
        if (token := self.peek()) and token[0] == expected:
            return self._buffer.popleft()
        return None

    def must_match(self, expected: Tokens) -> TokenType:
        if ret := self.may_match(expected):
            return ret
        raise SyntaxError()

    def peek(self) -> TokenType | None:
        if len(self._buffer) == 0:
            try:
                self._buffer.append(next(self.lexer))
            except StopIteration:
                return None
        return self._buffer[0]

    def skip_to_next_line(self):
        invalid, empty = (Tokens.INVALID, None), (Tokens.EMPTY, None)
        while self.peek() not in {invalid, empty, None}:
            self._buffer.popleft()
        # Consume trailing newline, so we can begin parsing on the next line
        if len(self._buffer) and self._buffer[0] == empty:
            self._buffer.popleft()

    def __next__(self) -> ParseTreeNode:
        if self.peek() is None:
            raise StopIteration()
        try:
            return self.statement()
        except SyntaxError as s:
            self.skip_to_next_line()
            return ErrorNode(error=s.msg if s.msg else None)
        except KeyError:
            self.skip_to_next_line()
            return ErrorNode()

    def macro(
        self, symbol: SymbolEntry | None = None
    ) -> MacroIR | None:
        if not (macro := self.may_match(Tokens.MACRO)):
            return None
        elif symbol is not None:
            raise SyntaxError("Macros cannot declare symbols")
        name = cast(str, macro[1])
        if (arg0 := self.argument()) is None:
            return None
        self.must_match(Tokens.COMMA)
        if (arg1 := self.argument()) is None:
            return None
        body = self.macro_registry.instantiate(
            name, str(arg0), str(arg1)
        )
        parse_tree = parse(body, self.symbol_table, self.macro_registry)
        return MacroIR(name, [arg0, arg1], parse_tree)

    def argument(self) -> ArgumentType | None:
        if _hex := self.may_match(Tokens.HEX):
            return Hexadecimal(cast(int, _hex[1]))
        elif dec := self.may_match(Tokens.DECIMAL):
            return Decimal(cast(int, dec[1]))
        elif ident := self.may_match(Tokens.IDENTIFIER):
            return Identifier(
                self.symbol_table.reference(cast(str, ident[1]))
            )
        elif str_const := self.may_match(Tokens.STRING):
            return StringConstant(cast(bytes, str_const[1]))
        return None

    def unary_instruction(
        self, symbol: SymbolEntry | None = None
    ) -> UnaryNode | None:
        if not (mn := self.may_match(Tokens.IDENTIFIER)):
            return None
        mn_str = cast(str, mn[1]).upper()
        if mn_str not in INSTRUCTION_TYPES:
            raise SyntaxError(f"Unrecognized mnemonic: {mn_str}")
        match INSTRUCTION_TYPES[mn_str]:
            case InstructionType.R | InstructionType.U:
                return UnaryIR(mn_str, sym=symbol)
        return None

    def nonunary_instruction(
        self, symbol: SymbolEntry | None = None
    ) -> NonUnaryNode | None:
        if not (mn := self.may_match(Tokens.IDENTIFIER)):
            return None
        mn_str = cast(str, mn[1]).upper()
        if mn_str not in INSTRUCTION_TYPES:
            raise SyntaxError(f"Unrecognized mnemonic: {mn_str}")
        match INSTRUCTION_TYPES[mn_str]:
            case InstructionType.R | InstructionType.U:
                self._buffer.appendleft(mn)
                return None
        if not (argument := self.argument()):
            self._buffer.appendleft(mn)
            return None

        if type(argument) == StringConstant and len(argument.value) > 2:
            raise SyntaxError("String too large")
        else:
            as_int = int(argument)
            try:
                as_int.to_bytes(2, signed=as_int < 0)
            except OverflowError:
                raise SyntaxError("Number too large")

        if self.may_match(Tokens.COMMA):
            # Check that addressing mode is a valid string and is allowed for the current mnemonic
            addr_str = cast(str, self.must_match(Tokens.IDENTIFIER)[1])
            addr_str = addr_str.upper()
            try:
                addr = cast(AddressingMode, AddressingMode[addr_str])
                mn_type = INSTRUCTION_TYPES[mn_str]
                if not mn_type.allows_addressing_mode(addr):
                    raise SyntaxError()
            except KeyError:
                raise SyntaxError()
            return NonUnaryIR(mn_str, argument, addr, sym=symbol)
        elif mn_str in DEFAULT_ADDRESSING_MODES:
            return NonUnaryIR(
                mn_str,
                argument,
                DEFAULT_ADDRESSING_MODES[mn_str],
                sym=symbol,
            )
        raise SyntaxError()

    def directive(
        self, symbol: SymbolEntry | None = None
    ) -> DotCommandIR | None:
        if not (dot := self.may_match(Tokens.DOT)):
            return None
        dot_str = cast(str, dot[1]).upper()
        argument: ArgumentType | None = None
        match dot_str:
            case "BYTE" | "WORD":
                if dec := self.may_match(Tokens.DECIMAL):
                    argument = Decimal(cast(int, dec[1]))
                elif _hex := self.may_match(Tokens.HEX):
                    argument = Hexadecimal(cast(int, _hex[1]))
                else:
                    message = f"{dot_str} requires an integer argument"
                    raise SyntaxError(message)
                width = 1 if dot_str == "BYTE" else 2
                return DotLiteralIR(argument, width=width, sym=symbol)

            case "ASCII":
                as_str = self.must_match(Tokens.STRING)
                argument = StringConstant(cast(bytes, as_str[1]))
                return DotASCIIIR(argument, sym=symbol)

            case "BLOCK":
                if dec := self.may_match(Tokens.DECIMAL):
                    argument = Decimal(cast(int, dec[1]))
                elif hex := self.may_match(Tokens.HEX):
                    argument = Hexadecimal(cast(int, hex[1]))
                else:
                    raise SyntaxError(
                        f"{dot_str} requires an integer argument"
                    )
                return DotBlockIR(argument, sym=symbol)

            case "EQUATE":
                if symbol is None:
                    message = ".EQUATE requires a symbol declaration"
                    raise SyntaxError(message)
                argument = self.argument()
                if argument is None:
                    raise SyntaxError(".EQUATE requires an argument")
                elif type(argument) == Identifier:
                    message = ".EQUATE requires a constant argument"
                    raise SyntaxError(message)
                else:
                    symbol.value = int(argument)
                return DotEquateIR(argument, sym=symbol)

            case _:
                raise SyntaxError(f"Unrecognized dot command {dot_str}")

    def code_line(
        self, symbol: SymbolEntry | None = None
    ) -> UnaryNode | NonUnaryNode | DotCommandIR | MacroNode | None:
        line: ParseTreeNode | None = None
        if nonunary := self.nonunary_instruction(symbol=symbol):
            line = nonunary
        elif unary := self.unary_instruction(symbol=symbol):
            line = unary
        elif (dot := self.directive(symbol=symbol)) is not None:
            line = dot
        elif (macro := self.macro(symbol=symbol)) is not None:
            line = macro
        else:
            return None

        if comment := self.may_match(Tokens.COMMENT):
            line.comment = cast(str, comment[1])
        return line

    def statement(self) -> ParseTreeNode:
        line: ParseTreeNode | None = None
        if self.may_match(Tokens.EMPTY):
            return EmptyIR()
        elif comment := self.may_match(Tokens.COMMENT):
            line = CommentIR(cast(str, comment[1]))
        elif symbol_token := self.may_match(Tokens.SYMBOL):
            symbol = self.symbol_table.define(
                cast(str, symbol_token[1])
            )
            if (code := self.code_line(symbol=symbol)) is None:
                message = "Symbol declaration must be followed \
                by instruction or dot command"
                raise SyntaxError(message)
            line = code
        elif (code := self.code_line()) is not None:
            line = code
        else:
            raise SyntaxError()

        self.must_match(Tokens.EMPTY)

        return line


def parse(
    text: str,
    symbol_table: SymbolTable | None = None,
    macro_registry: MacroRegistry | None = None,
) -> List[ParseTreeNode]:
    # Remove trailing whitespace while insuring input is \n terminated.
    parser = Parser(
        io.StringIO(text.rstrip() + "\n"),
        symbol_table=symbol_table,
        macro_registry=macro_registry,
    )
    return [line for line in parser]
