import itertools
from typing import List, Tuple, cast

from pep10.arguments import Identifier
from pep10.ir import Listable, listing, MacroIR, ErrorNode
from pep10.symbol import SymbolEntry
from pep10.types import ParseTreeNode, ArgumentType


def generate_code(
    parse_tree: List[ParseTreeNode], base_address=0
) -> Tuple[List[Listable], List[str]]:
    errors: List[str] = []
    ir: List[Listable] = []
    address = base_address
    for node in parse_tree:
        if isinstance(node, ErrorNode):
            errors.append(node.source())
            continue
        elif isinstance(node, MacroIR) and isinstance(node, Listable):
            inner = generate_code(node.body, base_address=address)
            ir.append(node.start_comment())
            ir.extend(inner[0])
            ir.append(node.end_comment())
            errors.extend(inner[1])
        elif isinstance(node, Listable):
            ir.append(node)
        else:
            continue

        line: Listable = cast(Listable, node)
        # The size of the IR line may depend on the address, e.g., .ALIGN
        line.address = address
        # Check for multiply defined symbols, and assign addresses to symbol declarations
        if maybe_symbol := getattr(node, "symbol_decl", None):
            symbol: SymbolEntry = maybe_symbol
            if symbol.is_multiply_defined():
                errors.append(f"Multiply defined symbol: {symbol}")
            elif len(line) > 0:
                # Avoid re-assigning symbol values for .EQUATEs
                symbol.value = address
        # Check that symbols used as arguments are not undefined.
        if maybe_argument := getattr(line, "argument", None):
            argument: ArgumentType = maybe_argument
            if (
                type(argument) is Identifier
                and argument.symbol.is_undefined()
            ):
                errors.append(f"Undefined symbol: {argument.symbol}")
        address += len(line)

    return ir, errors


def program_object_code(program: List[Listable]) -> bytes:
    return b"".join(line.object_code() for line in program)


def program_source(program: List[Listable]) -> List[str]:
    return [line.source() for line in program]


def program_listing(program: List[Listable]) -> List[str]:
    return list(
        itertools.chain.from_iterable(listing(line) for line in program)
    )
