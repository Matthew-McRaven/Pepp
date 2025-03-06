from pep10.code_gen import program_object_code, generate_code
from pep10.ir import CommentIR, EmptyIR
from pep10.parser import parse
from pep10.symbol import SymbolTable


def test_unary_object_code():
    parse_tree = parse("NOTA\nNOTA\nRET\n")
    ir, errors = generate_code(parse_tree)
    assert len(errors) == 0
    assert len(parse_tree) == 3 and len(ir) == 3
    assert program_object_code(ir) == bytes([0x18, 0x18, 0x1])
    assert ir[0].address == 0 and ir[1].address == 1 and ir[2].address == 2


def test_nonunary_object_code():
    st = SymbolTable()
    parse_tree = parse("cat:BR 3,i\ndog:ADDA 0x10,d\nCALL cat,i\n", symbol_table=st)
    ir, errors = generate_code(parse_tree)
    assert len(errors) == 0
    assert "cat" in st and int(st["cat"]) == 0
    assert "dog" in st and int(st["dog"]) == 3
    assert len(parse_tree) == 3 and len(ir) == 3
    assert program_object_code(ir) == bytes(
        [0x24, 0x00, 0x03, 0x51, 0x00, 0x10, 0x36, 0x00, 0x00]
    )
    assert ir[0].address == 0 and ir[1].address == 3 and ir[2].address == 6


def test_comment_empty():
    parse_tree = parse("\n;test comment")
    ir, errors = generate_code(parse_tree)
    assert len(errors) == 0

    assert len(parse_tree) == 2 and len(ir) == 2
    assert program_object_code(ir) == bytes()
    assert type(ir[0]) is EmptyIR and type(ir[1]) is CommentIR
