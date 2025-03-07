import argparse
import sys

from pep10.code_gen import (
    generate_code,
    program_object_code,
    program_listing,
    program_source,
)
from pep10.ir import ErrorNode
from pep10.macro import MacroRegistry, add_OS_macros
from pep10.parser import parse
from pep10.symbol import SymbolTable, add_OS_symbols


def assemble(text: str):
    st, mr = SymbolTable(), MacroRegistry()
    add_OS_symbols(st), add_OS_macros(mr)
    parse_tree = parse(text, symbol_table=st, macro_registry=mr)
    parse_errors = list(
        filter(lambda n: type(n) is ErrorNode, parse_tree)
    )
    if len(parse_errors) > 0:
        for error in parse_errors:
            print(error, file=sys.stderr)
        raise SyntaxError("Failed to parse program")
    ir, ir_errors = generate_code(parse_tree)
    if len(ir_errors) > 0:
        for ir_error in ir_errors:
            print(ir_error, file=sys.stderr)
        raise SyntaxError("Failed to generate object code")
    return (
        program_object_code(ir),
        program_listing(ir),
        program_source(ir),
    )


def main():
    # Call with no arguments to open the GUI, or with a single file argument to run in the terminal.
    parser = argparse.ArgumentParser(description="Fig 07.38")
    parser.add_argument("input_file", default=None)
    args = parser.parse_args()
    if args.input_file:
        with open(args.input_file, "r") as f:
            code, listing, source = assemble("".join(f.readlines()))
            print("\n".join(listing))


if __name__ == "__main__":
    main()
