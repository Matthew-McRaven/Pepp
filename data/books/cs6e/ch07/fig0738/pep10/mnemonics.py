from enum import Enum
from typing import Dict


class AddressingMode(Enum):
    I, D, N, S, SF, X, SX, SFX = range(8)

    def as_AAA(self) -> int:
        return self.value

    def as_A(self) -> int:
        match self.value:
            case AddressingMode.I.value:
                return 0
            case AddressingMode.X.value:
                return 1
        message = f"Invalid addressing mode for A type: {self.name}"
        raise TypeError(message)


class InstructionType(Enum):
    U = "U"
    R = "R"
    A_ix = "A_ix"
    AAA_all = "AAA_all"
    AAA_i = "AAA_i"
    RAAA_all = "RAAA_all"
    RAAA_noi = "RAAA_noi"

    def allows_addressing_mode(self, am: AddressingMode):
        # Default to no allowed addressing modes
        return am in {
            "A_ix": {AddressingMode.I, AddressingMode.X},
            "AAA_all": {am for am in AddressingMode},
            "AAA_i": {AddressingMode.I},
            "RAAA_all": {am for am in AddressingMode},
            "RAAA_noi": {am for am in AddressingMode}
            - {AddressingMode.I},
        }.get(self.value, {})


INSTRUCTION_TYPES: Dict[str, InstructionType] = {
    "RET": InstructionType.U,
    # SRET, MOVFLGA, MOVAFLG, MOVSPA, MOVASP
    "NOP": InstructionType.U,
    "NOTA": InstructionType.R,
    "NOTX": InstructionType.R,
    # NEGr, ASLr, ASRr, ROLr, RORr
    "BR": InstructionType.A_ix,
    # BR(LE|LT|EQ|NE|GE|GT|V|C)
    "CALL": InstructionType.A_ix,
    "SCALL": InstructionType.AAA_all,
    # (ADD|SUB)SP
    "ADDA": InstructionType.RAAA_all,
    "ADDX": InstructionType.RAAA_all,
    # (SUB|AND|OR|XOR)r
    "CPWA": InstructionType.RAAA_all,
    "CPWX": InstructionType.RAAA_all,
    # CPBr
    "LDWA": InstructionType.RAAA_all,
    "LDWX": InstructionType.RAAA_all,
    # LDBr
    "STWA": InstructionType.RAAA_noi,
    "STWX": InstructionType.RAAA_noi,
    # STBr
}

DEFAULT_ADDRESSING_MODES: Dict[str, AddressingMode] = {
    "BR": AddressingMode.I,
    # BR(LE|LT|EQ|NE|GE|GT|V|C)
    "CALL": AddressingMode.I,
}


BITS: Dict[str, int] = {
    "RET": 0x01,
    # SRET, MOVFLGA, MOVAFLG, MOVSPA, MOVASP
    "NOP": 0x07,
    "NOTA": 0x18,
    "NOTX": 0x19,
    # NEGr, ASLr, ASRr, ROLr, RORr
    "BR": 0x24,
    # BR(LE|LT|EQ|NE|GE|GT|V|C)
    "CALL": 0x36,
    "SCALL": 0x38,
    # (ADD|SUB)SP
    "ADDA": 0x50,
    "ADDX": 0x58,
    # (SUB|AND|OR|XOR)r
    "CPWA": 0xA0,
    "CPWX": 0xA8,
    # STBr
    "LDWA": 0xC0,
    "LDWX": 0xC8,
    # LDBr
    "STWA": 0xE0,
    "STWX": 0xE8,
    # STBr
}


def as_int(mnemonic: str, am: AddressingMode | None = None) -> int:
    mnemonic = mnemonic.upper()
    bit_pattern, mn_type = BITS[mnemonic], INSTRUCTION_TYPES[mnemonic]

    if mn_type == InstructionType.U or mn_type == InstructionType.R:
        return bit_pattern
    elif mn_type == InstructionType.A_ix:
        return bit_pattern | (0 if am is None else am.as_A())
    else:
        return bit_pattern | (0 if am is None else am.as_AAA())
