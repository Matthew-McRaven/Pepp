from lang.mnemonics import Mnemonics


def test_to_string():
    assert str(Mnemonics.ADD) == "add"
    assert str(Mnemonics.SUB) == "sub"
    assert str(Mnemonics.MUL) == "mul"
    assert str(Mnemonics.DIV) == "div"
    assert str(Mnemonics.ABS) == "abs"
    assert str(Mnemonics.SET) == "set"
    assert str(Mnemonics.STOP) == "stop"
    assert str(Mnemonics.END) == "end"


def test_is_unary():
    assert not Mnemonics.ADD.is_unary()
    assert not Mnemonics.SUB.is_unary()
    assert not Mnemonics.MUL.is_unary()
    assert not Mnemonics.DIV.is_unary()
    assert not Mnemonics.NEG.is_unary()
    assert not Mnemonics.ABS.is_unary()
    assert not Mnemonics.SET.is_unary()
    assert Mnemonics.STOP.is_unary()
    assert Mnemonics.END.is_unary()


def test_is_nonunary():
    assert Mnemonics.ADD.is_nonunary()
    assert Mnemonics.SUB.is_nonunary()
    assert Mnemonics.MUL.is_nonunary()
    assert Mnemonics.DIV.is_nonunary()
    assert Mnemonics.NEG.is_nonunary()
    assert Mnemonics.ABS.is_nonunary()
    assert Mnemonics.SET.is_nonunary()
    assert not Mnemonics.STOP.is_nonunary()
    assert not Mnemonics.END.is_nonunary()
