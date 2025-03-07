import pytest

from pep10.mnemonics import AddressingMode, InstructionType, as_int


def test_AAA_bit_patterns():
    assert AddressingMode.I.as_AAA() == 0
    assert AddressingMode.D.as_AAA() == 1
    assert AddressingMode.N.as_AAA() == 2
    assert AddressingMode.S.as_AAA() == 3
    assert AddressingMode.SF.as_AAA() == 4
    assert AddressingMode.X.as_AAA() == 5
    assert AddressingMode.SX.as_AAA() == 6
    assert AddressingMode.SFX.as_AAA() == 7


def test_A_bit_patterns():
    assert AddressingMode.I.as_A() == 0
    assert AddressingMode.X.as_A() == 1
    with pytest.raises(TypeError):
        AddressingMode.SX.as_A()


def test_type_masks():
    assert InstructionType.U != InstructionType.R
    # Mask calculations work
    assert InstructionType.A_ix.allows_addressing_mode(AddressingMode.I)
    assert InstructionType.A_ix.allows_addressing_mode(AddressingMode.X)
    assert not InstructionType.A_ix.allows_addressing_mode(
        AddressingMode.D
    )


def test_U_mnemonics():
    assert as_int("RET") == 0x1


def test_R_mnemonics():
    assert as_int("NOTA") == 0x18


def test_A_mnemonics():
    assert as_int("call") == 0x36
    assert as_int("call", am=AddressingMode.I) == 0x36
    assert as_int("call", am=AddressingMode.X) == 0x37
