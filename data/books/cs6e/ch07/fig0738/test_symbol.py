from pep10.symbol import SymbolTable, add_OS_symbols


# Referring to the same symbol in different places accesses the same underlying object.
def test_same_object():
    tb = SymbolTable()
    s0 = tb.reference("test")
    s1 = tb.reference("test")
    assert s0 == s1
    s2 = tb.define("test")
    assert s0 == s2
    assert s2 == tb.define("test")


def test_undefined():
    tb = SymbolTable()
    s = tb.reference("test")
    assert s.is_undefined()
    assert not s.is_multiply_defined()


def test_definition_transitions():
    tb = SymbolTable()
    s0 = tb.reference("test")
    assert s0.is_undefined()
    assert not s0.is_multiply_defined()
    tb.define("test")
    assert not s0.is_undefined()
    assert not s0.is_multiply_defined()
    # Ensure that repeated definitions keeps us in the multiply defined state.
    for i in range(3):
        tb.define("test")
        assert not s0.is_undefined()
        assert s0.is_multiply_defined()


def test_value_assignment():
    tb = SymbolTable()
    s0 = tb.reference("test")
    assert s0.value is None
    assert int(s0) == 0
    s0.value = 5
    assert s0.value == 5
    assert int(s0) == 5


def tst_os_symbols():
    tb = SymbolTable()
    add_OS_symbols(tb)
    assert "charIn" in tb and "charOut" in tb and "pwrOff" in tb
    for s in {"DECI", "DECO", "STRO", "HEXO", "SNOP"}:
        assert s in tb
