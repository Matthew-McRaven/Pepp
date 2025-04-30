import argparse, copy, enum, io
import leb128
class Registers:
    def __init__(self, address=0, line=1):
        self.address = address
        self.line = line
        self.file = 0
        self.column = 0
        self.is_stmt = True
        self.basic_block = False
        self.end_sequence = False
        self.prologue_end = False
        self.epilogue_begin = False

    def __repr__(self):
        return f"Registers(address={self.address:04x}, line={self.line}, file={self.file}, column={self.column}, is_stmt={self.is_stmt}, basic_block={self.basic_block}, end_sequence={self.end_sequence}, prologue_end={self.prologue_end}, epilogue_begin={self.epilogue_begin})"
    def __str__(self):
        flc = f"{self.file}:{self.line}:{self.column:}"
        return f"Registers({self.address:04x}, {flc:>10}, is_stmt={1 if self.is_stmt else 0})"

class NonSpecialOpcodes(enum.Enum):
    COPY = 0
    ADVANCE_PC = 1
    ADVANCE_LINE = 2
    SET_FILE = 3
    SET_COLUMN = 4
    NEGATE_STMT = 5
    SET_BASIC_BLOCK = 6
    CONST_ADD_PC = 7
    FIXED_ADVANCE_PC = 8
    SET_PROLOGUE_END = 9
    SET_EPILOGUE_BEGIN = 10
    SET_ISA = 11
    END_SEQUENCE = 12
    SET_ADDRESS = 13

class LineNumberTable:
    def __init__(self, line_base, line_range):
        self.line_base = line_base
        self.line_range = line_range
        self.maximum_operations_per_instruction = 1
        self.minimum_instruction_length = 1
        self.opcode_base = 14
        self.standard_opcode_lengths = [0,1,1,1,1,0,0,0,1,0,0,1,0,1]
        self.regs = Registers()
        self._table = []

    def crack_special(self, opcode):
        adjusted_opcode = opcode - self.opcode_base
        operation_advance = adjusted_opcode // self.line_range
        address_increment = self.minimum_instruction_length * operation_advance
        line_increment = self.line_base + adjusted_opcode % self.line_range
        return address_increment, line_increment

    def compute_special(self, address_increment, line_increment):
        if line_increment - self.line_base >= self.line_range: return None
        operation_advance = address_increment // self.minimum_instruction_length
        adjusted_opcode = operation_advance * self.line_range + (line_increment - self.line_base)
        opcode = adjusted_opcode + self.opcode_base
        if adjusted_opcode < 256 - self.opcode_base: return opcode
        else: return None

    def execute_opcode(self, opcode, arg=None, describe=False):
        old_regs = self.regs
        self.regs = copy.copy(self.regs)
        match opcode:
            case NonSpecialOpcodes.COPY.value:
                self._table.append(copy.copy(self.regs))
                self.regs.basic_block = False
                self.regs.prologue_end = False
                self.regs.epilogue_begin = False
            case NonSpecialOpcodes.ADVANCE_PC.value:
                self.regs.address += self.minimum_instruction_length * arg
            case NonSpecialOpcodes.ADVANCE_LINE.value: self.regs.line += arg
            case NonSpecialOpcodes.SET_FILE.value: self.regs.file = arg
            case NonSpecialOpcodes.SET_COLUMN.value: self.regs.column = arg
            case NonSpecialOpcodes.NEGATE_STMT.value: self.regs.is_stmt = not self.regs.is_stmt
            case NonSpecialOpcodes.SET_BASIC_BLOCK.value: self.regs.basic_block = True
            case NonSpecialOpcodes.CONST_ADD_PC.value:
                address_increment, _ = self.crack_special(255)
                self.regs.address += address_increment
            case NonSpecialOpcodes.FIXED_ADVANCE_PC.value: self.regs.address += arg
            case NonSpecialOpcodes.SET_PROLOGUE_END.value: self.regs.prologue_end = True;
            case NonSpecialOpcodes.SET_EPILOGUE_BEGIN.value: self.regs.epilogue_begin = True
            case NonSpecialOpcodes.SET_ISA.value: pass
            case NonSpecialOpcodes.END_SEQUENCE.value: self.regs.end_sequence = True
            case NonSpecialOpcodes.SET_ADDRESS.value:
                assert arg is not None
                self.regs.address = arg
            case _:
                address_increment, line_increment = self.crack_special(opcode)
                self.regs.line += line_increment
                self.regs.address += address_increment
                self._table.append(copy.copy(self.regs))
                self.regs.basic_block = False
                self.regs.prologue_end = False
                self.regs.epilogue_begin = False
        if describe: print(self.describe(opcode, arg))
        return old_regs

    def execute_program(self, program, describe=False):
        index = 0
        buf = io.BytesIO(program)
        while (opcode := buf.read(1)):
            opcode = ord(opcode)
            if opcode < self.opcode_base and self.standard_opcode_lengths[opcode] == 0: self.execute_opcode(opcode, describe=describe)
            elif opcode >= self.opcode_base: self.execute_opcode(opcode, describe=describe)
            elif opcode == NonSpecialOpcodes.ADVANCE_LINE.value:
                argument, index = leb128.i.decode_reader(buf)
                self.execute_opcode(opcode, argument, describe=describe)
            else:
                argument, index = leb128.u.decode_reader(buf)
                self.execute_opcode(opcode, argument, describe=describe)


    def describe(self, opcode, argument=None):
        match opcode:
            case NonSpecialOpcodes.COPY.value: return f"Copy {self.regs}"
            case NonSpecialOpcodes.ADVANCE_PC.value: return f"Advance PC by {argument} to {self.regs.address:04X}"
            case NonSpecialOpcodes.ADVANCE_LINE.value: return f"Advance Line by {argument} to {self.regs.line}"
            case NonSpecialOpcodes.SET_FILE.value: return f"Set column to {argument}"
            case NonSpecialOpcodes.SET_COLUMN.value: return f"Set column to {argument}"
            case NonSpecialOpcodes.NEGATE_STMT.value: return f"Set is_stmt to {self.regs.is_stmt}"
            case NonSpecialOpcodes.SET_BASIC_BLOCK.value: return f"Set basic_block to True"
            case NonSpecialOpcodes.CONST_ADD_PC.value:
                address_increment, _ = self.crack_special(255)
                return f"Advance PC by constant {address_increment} to {self.regs.address:04X}"
            case NonSpecialOpcodes.FIXED_ADVANCE_PC.value: return f"Advance PC by fixed {argument} to {self.regs.address:04X}"
            case NonSpecialOpcodes.SET_PROLOGUE_END.value: return f"Set prologue_end to True"
            case NonSpecialOpcodes.SET_EPILOGUE_BEGIN.value: return f"Set epilogue_begin to True"
            case NonSpecialOpcodes.SET_ISA.value: pass
            case NonSpecialOpcodes.END_SEQUENCE.value: return f"Extended opcode 1: End of Sequence"
            case NonSpecialOpcodes.SET_ADDRESS.value: return f"Set Address to {argument:04X}"
            case _:
                address_increment, line_increment = self.crack_special(opcode)
                return f"Special opcode {opcode:2x}: advance Address by {address_increment} to {self.regs.address:04X} and Line by {line_increment} to {self.regs.line}"

def serialize_op(opcode, arg=None):
    """Serialize the opcode and argument into a bytearray."""
    r = bytearray()
    if isinstance(opcode, NonSpecialOpcodes): opcode = int(opcode.value)

    r.extend(opcode.to_bytes(1))

    if arg is None: return r
    if opcode == NonSpecialOpcodes.FIXED_ADVANCE_PC.value: r.extend(arg.to_bytes(2, 'little'))
    else: r.extend(leb128.u.encode(arg))
    return r

def serialize_program(program):
    r = bytearray()
    for opcode, arg in program: r.extend(serialize_op(opcode, arg))
    return r

OS = [
    (0xFA82, 6), (0xFB02, 7), (0xFB03, 8), (0xFB04, 9), (0xFB06, 10), (0xFB08, 11),
    (0xFB0A, 22)
]

def program_from_offsets(address_lines, line_base, line_range):
    s = LineNumberTable(line_base, line_range)
    program = []
    base_addr, base_list = address_lines[0]
    program.append((NonSpecialOpcodes.SET_ADDRESS, base_addr))
    program.append((NonSpecialOpcodes.COPY, None))
    for address, list in address_lines[1:]:
        address_increment, line_increment = address - base_addr,  list - base_list
        maybe_opcode = s.compute_special(address_increment, line_increment)
        #print((address,list), (address_increment, line_increment), maybe_opcode)
        if maybe_opcode is not None: program.append((maybe_opcode, None))
        else:
            program.append((NonSpecialOpcodes.ADVANCE_PC, address_increment))
            program.append((NonSpecialOpcodes.ADVANCE_LINE, line_increment))
            program.append((NonSpecialOpcodes.COPY, None))
        base_addr, list = address, list
    return program


def main(line_base=0, line_range=0):
    s = LineNumberTable(line_base, line_range)
    """for opcode in range(14,256):
        cracked = s.crack_special(opcode)
        print(opcode, cracked, s.compute_special(*cracked))"""

    program = program_from_offsets(OS, line_base, line_range)

    prog_bytes = serialize_program(program)
    print("Program: "+' '.join(f"{x:02x}" for x in prog_bytes))
    s.execute_program(prog_bytes, describe=False)
    print("Registers:")
    for i, r in enumerate(s._table): print(f"{i:2}: {r}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--line-base', type=int, default=0)
    parser.add_argument('--line-range', type=int, default=7)
    args = parser.parse_args()
    main(**vars(args))
