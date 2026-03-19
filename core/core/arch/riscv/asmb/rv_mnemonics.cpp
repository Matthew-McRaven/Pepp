#include "core/arch/riscv/asmb/rv_mnemonics.hpp"
#include "core/arch/riscv/asmb/rvi_patterns.hpp"
#include "core/arch/riscv/isa/rv_instruction_list.hpp"

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::I(u8 opcode7, u8 funct3) {
  MnemonicDescriptor ret(Type::I, opcode7);
  ret._funct3 = funct3 & 0x7;
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::IShiftByConstant(u8 opcode7, u8 funct3, u8 shift_type) {
  MnemonicDescriptor ret(Type::I, opcode7);
  ret._funct3 = funct3 & 0x7;
  // For shift instructions, the imm field is used to encode the shift type (logical vs arithmetic) in bit 10.
  ret._imm_or_funct7 = (shift_type & 0x1) << 10;
  ret._flags.imm = 1;
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::IFence(u8 fmt) {
  MnemonicDescriptor ret(Type::I, RV32I_FENCE);
  ret._funct3 = 0;
  // fmt is high-order 4 bits of imm
  ret._imm_or_funct7 = (fmt & 0b1111) << 8;
  ret._flags.imm = 1;
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::IFence(u8 fmt, u8 pred, u8 succ) {
  MnemonicDescriptor ret(Type::I, RV32I_FENCE);
  ret._funct3 = 0;
  // fmt is high-order 4 bits of imm, pred middle 4 bits, and succ low-order 4 bits.
  ret._imm_or_funct7 = (fmt & 0b1111) << 8 | (pred & 0b1111) << 4 | (succ & 0b1111);
  ret._flags.imm = 1;
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::R(u8 opcode7, u8 funct3, u8 funct7) {
  MnemonicDescriptor ret(Type::R, opcode7);
  ret._funct3 = funct3 & 0x7;
  ret._imm_or_funct7 = funct7 & 0x7F;
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::S(u8 opcode7, u8 funct3) {
  MnemonicDescriptor ret(Type::S, opcode7);
  ret._funct3 = funct3 & 0x7;
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::B(u8 opcode7, u8 funct3) {
  MnemonicDescriptor ret(Type::B, opcode7);
  ret._funct3 = funct3 & 0x7;
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::U(u8 opcode7) { return MnemonicDescriptor(Type::U, opcode7); }

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::J(u8 opcode7) { return MnemonicDescriptor(Type::J, opcode7); }

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::Pseudo() {
  MnemonicDescriptor ret(Type::Pseudo);
  return ret;
}

std::span<const riscv::Operand> riscv::MnemonicDescriptor::operands() const noexcept {
  int it = 0;
  for (; it < _operands.size(); it++) {
    if (_operands[it].type == Operand::Type::Invalid) break;
  }
  return std::span<const Operand>(_operands.data(), it);
}

riscv::MnemonicDescriptor::Type riscv::MnemonicDescriptor::type() const noexcept { return _type; }

void riscv::MnemonicDescriptor::append_operand(Operand operand) {
  int it = 0;
  for (; it < _operands.size(); it++) {
    if (_operands[it].type == Operand::Type::Invalid) {
      _operands[it] = operand;
      return;
    }
  }
  if (it == _operands.size()) throw std::runtime_error("Too many operands for this mnemonic");
}

riscv::MnemonicDescriptor &&riscv::MnemonicDescriptor::with_operand(Operand first,
                                                                    std::same_as<Operand> auto... rest) && {
  append_operand(first);
  if constexpr (sizeof...(rest) > 0) return std::move(*this).with_operand(rest...);
  return std::move(*this);
}

riscv::MnemonicDescriptor
riscv::MnemonicDescriptor::replaced_operands(std::same_as<Operand> auto... ops) const noexcept {
  static const Operand invalid{.type = Operand::Type::Invalid, .destination = Operand::Destination::Invalid};
  MnemonicDescriptor ret = *this;
  ret._operands.fill(invalid);
  return std::move(ret).with_operand(ops...);
}

bool riscv::MnemonicDescriptor::allows_rs1() const noexcept {
  switch (_type) {
  case Type::INVALID: return false;
  case Type::R: return true;
  case Type::I: return true;
  case Type::S: return true;
  case Type::B: return true;
  case Type::U: return false;
  case Type::J: return false;
  case Type::Pseudo: {
    for (const auto &operand : operands())
      if (operand.destination == Operand::Destination::RS1) return true;
    return false;
  }
  }
  return false;
}

bool riscv::MnemonicDescriptor::has_rs1() const noexcept { return _flags.rs1; }

void riscv::MnemonicDescriptor::set_rs1(u8 rs1) {
  _rs1 = rs1 & (1 << 5) - 1;
  _flags.rs1 = 1;
}

std::optional<u8> riscv::MnemonicDescriptor::get_rs1() const {
  return has_rs1() ? std::optional<u8>(_rs1) : std::nullopt;
}

riscv::MnemonicDescriptor &&riscv::MnemonicDescriptor::with_rs1(u8 rs1) && {
  set_rs1(rs1);
  return std::move(*this);
}

bool riscv::MnemonicDescriptor::allows_rs2() const noexcept {
  switch (_type) {
  case Type::INVALID: return false;
  case Type::R: return true;
  case Type::I: return false;
  case Type::S: return true;
  case Type::B: return true;
  case Type::U: return false;
  case Type::J: return false;
  case Type::Pseudo: {
    for (const auto &operand : operands())
      if (operand.destination == Operand::Destination::RS2) return true;
    return false;
  }
  }
  return false;
}

bool riscv::MnemonicDescriptor::has_rs2() const noexcept { return _flags.rs2; }

void riscv::MnemonicDescriptor::set_rs2(u8 rs2) {
  _rs2 = rs2 & (1 << 5) - 1;
  _flags.rs2 = 1;
}

std::optional<u8> riscv::MnemonicDescriptor::get_rs2() const {
  return has_rs2() ? std::optional<u8>(_rs2) : std::nullopt;
}

riscv::MnemonicDescriptor &&riscv::MnemonicDescriptor::with_rs2(u8 rs2) && {
  set_rs2(rs2);
  return std::move(*this);
}

bool riscv::MnemonicDescriptor::allows_rd() const noexcept {
  switch (_type) {
  case Type::INVALID: return false;
  case Type::R: return true;
  case Type::I: return true;
  case Type::S: return false;
  case Type::B: return false;
  case Type::U: return true;
  case Type::J: return true;
  case Type::Pseudo: {
    for (const auto &operand : operands())
      if (operand.destination == Operand::Destination::RD) return true;
    return false;
  }
  }
  return false;
}

bool riscv::MnemonicDescriptor::has_rd() const noexcept { return _flags.rd; }

void riscv::MnemonicDescriptor::set_rd(u8 rd) {
  _rd = rd & (1 << 5) - 1;
  _flags.rd = 1;
}

std::optional<u8> riscv::MnemonicDescriptor::get_rd() const { return has_rd() ? std::optional<u8>(_rd) : std::nullopt; }

riscv::MnemonicDescriptor &&riscv::MnemonicDescriptor::with_rd(u8 rd) && {
  set_rd(rd);
  return std::move(*this);
}

bool riscv::MnemonicDescriptor::allows_funct3() const noexcept {
  switch (_type) {
  case Type::INVALID: return false;
  case Type::R: return true;
  case Type::I: return true;
  case Type::S: return true;
  case Type::B: return true;
  case Type::U: return false;
  case Type::J: return false;
  case Type::Pseudo: return false;
  }
  return false;
}

bool riscv::MnemonicDescriptor::allows_funct7() const noexcept { return _type == Type::R; }

bool riscv::MnemonicDescriptor::allows_imm() const noexcept {
  switch (_type) {
  case Type::INVALID: return false;
  case Type::R: return false;
  case Type::I: return true;
  case Type::S: return true;
  case Type::B: return true;
  case Type::U: return true;
  case Type::J: return true;
  case Type::Pseudo: {
    for (const auto &operand : operands())
      if (operand.destination == Operand::Destination::IMM) return true;
    return false;
  }
  }
  return false;
}

bool riscv::MnemonicDescriptor::has_imm() const noexcept { return allows_imm() && _flags.imm; }

void riscv::MnemonicDescriptor::set_imm(u32 imm) {
  switch (_type) {
  // Bits [11:0]
  case Type::I: [[fallthrough]];
  case Type::S:
    _imm_or_funct7 = imm & ((1 << 12) - 1);
    break;
    // Bits [12:1]
  case Type::B: _imm_or_funct7 = ((imm >> 1u) & ((1 << 11) - 1)); break;
  // Upper [31:12] bits
  case Type::U:
    _imm_or_funct7 = (imm >> 12u) & ((1 << 20) - 1);
    break;
    // Bits [20:1]
  case Type::J: _imm_or_funct7 = (imm >> 1u) & ((1 << 20) - 1); break;
  case Type::R: [[fallthrough]];
  case Type::INVALID: [[fallthrough]];
  case Type::Pseudo: [[fallthrough]];
  default: throw std::runtime_error("This mnemonic does not allow an immediate");
  }
}

std::optional<u32> riscv::MnemonicDescriptor::get_imm() const {
  return has_imm() ? std::optional<u32>(_imm_or_funct7) : std::nullopt;
}

u8 riscv::MnemonicDescriptor::width_imm() const noexcept {
  switch (_type) {
  case Type::INVALID: return 0;
  case Type::R: return 0;
  case Type::I: return 12;
  case Type::S: return 12;
  case Type::B: return 12;
  case Type::U: return 20;
  case Type::J: return 20;
  case Type::Pseudo: return 0;
  }
}

riscv::MnemonicDescriptor &&riscv::MnemonicDescriptor::with_imm(u32 imm) && {
  set_imm(imm);
  return std::move(*this);
}

riscv::MnemonicDescriptor::MnemonicDescriptor(Type type, u8 opcode) : _type(type), _opcode7(opcode) {}

template <> riscv::InstructionR riscv::MnemonicDescriptor::encode<riscv::InstructionR>(Values v) const {
  const u8 rs1 = v.rs1.value_or(_rs1) & 0x1F;
  const u8 rs2 = v.rs2.value_or(_rs2) & 0x1F;
  const u8 rd = v.rd.value_or(_rd) & 0x1F;
  const u8 funct7 = _imm_or_funct7 & 0x7F;
  return InstructionR{.opcode = _opcode7, .rd = rd, .funct3 = _funct3, .rs1 = rs1, .rs2 = rs2, .funct7 = funct7};
}
template <> riscv::InstructionI riscv::MnemonicDescriptor::encode<riscv::InstructionI>(Values v) const {
  const u8 rs1 = v.rs1.value_or(_rs1) & 0x1F;
  const u8 rd = v.rd.value_or(_rd) & 0x1F;
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u16 imm = (v.imm.value_or(0) | _imm_or_funct7) & 0xFFF;
  return InstructionI{.opcode = _opcode7, .rd = rd, .funct3 = _funct3, .rs1 = rs1, .imm = imm};
}
template <> riscv::InstructionS riscv::MnemonicDescriptor::encode<riscv::InstructionS>(Values v) const {
  const u8 rs1 = v.rs1.value_or(_rs1) & 0x1F;
  const u8 rs2 = v.rs2.value_or(_rs2) & 0x1F;
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u16 imm = (v.imm.value_or(0) | _imm_or_funct7) & ((1 << 12) - 1);
  const u8 imm11_05 = (imm >> 5u) & ((1 << 7) - 1);
  const u8 imm4_0 = imm & ((1 << 5) - 1);
  return InstructionS{.opcode = _opcode7, .imm1 = imm4_0, .funct3 = _funct3, .rs1 = rs1, .rs2 = rs2, .imm2 = imm11_05};
}
template <> riscv::InstructionU riscv::MnemonicDescriptor::encode<riscv::InstructionU>(Values v) const {
  const u8 rd = v.rd.value_or(_rd) & 0x1F;
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u32 imm = (v.imm.value_or(0) | _imm_or_funct7) & ((1 << 20) - 1);
  return InstructionU{.opcode = _opcode7, .rd = rd, .imm = imm};
}
template <> riscv::InstructionB riscv::MnemonicDescriptor::encode<riscv::InstructionB>(Values v) const {
  const u8 rs1 = v.rs1.value_or(_rs1) & 0x1F;
  const u8 rs2 = v.rs2.value_or(_rs2) & 0x1F;
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u16 imm = (v.imm.value_or(0) | _imm_or_funct7) & ((1 << 12) - 1);
  const u8 imm12_12 = (imm >> 12u) & 1;
  const u8 imm10_5 = (imm >> 5u) & ((1 << 6) - 1);
  const u8 imm4_1 = (imm >> 1u) & ((1 << 4) - 1);
  const u8 imm11_11 = (imm >> 11u) & 1;
  return InstructionB{.opcode = _opcode7,
                      .imm1 = imm4_1,
                      .imm2 = imm10_5,
                      .funct3 = _funct3,
                      .rs1 = rs1,
                      .rs2 = rs2,
                      .imm3 = imm11_11,
                      .imm4 = imm12_12};
}
template <> riscv::InstructionJ riscv::MnemonicDescriptor::encode<riscv::InstructionJ>(Values v) const {
  const u8 rd = v.rd.value_or(_rd) & 0x1F;
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u32 imm = (v.imm.value_or(0) | _imm_or_funct7) & ((1 << 20) - 1);
  const u8 imm20_20 = (imm >> 20u) & 1;
  const u16 imm10_01 = (imm >> 1u) & ((1 << 10) - 1);
  const u8 imm11_11 = (imm >> 11u) & 1;
  const u16 imm19_12 = (imm >> 12u) & ((1 << 8) - 1);
  return InstructionJ{
      .opcode = _opcode7, .rd = rd, .imm1 = imm19_12, .imm2 = imm20_20, .imm3 = imm10_01, .imm4 = imm11_11};
}

static void add_rv32i_instructions(riscv::MnemonicSet &mn_set) {
  using namespace riscv;
  auto add = [&](riscv::Mnemonic mn) { mn_set.insert(mn); };
  add({"lui", LUI});
  add({"auipc", AUIPC});
  add({"jal", JAL});
  add({"jalr", JALR});
  add({"beq", BEQ});
  add({"bne", BNE});
  add({"blt", BLT});
  add({"bge", BGE});
  add({"bltu", BLTU});
  add({"bgeu", BGEU});
  add({"lb", LB});
  add({"lh", LH});
  add({"lw", LW});
  add({"lbu", LBU});
  add({"lhu", LHU});
  add({"sb", SB});
  add({"sh", SH});
  add({"sw", SW});
  add({"addi", ADDI});
  add({"slti", SLTI});
  add({"sltiu", SLTIU});
  add({"xori", XORI});
  add({"ori", ORI});
  add({"andi", ANDI});
  add({"slli", SLLI});
  add({"srli", SRLI});
  add({"srai", SRAI});
  add({"add", ADD});
  add({"sub", SUB});
  add({"sll", SLL});
  add({"slt", SLT});
  add({"sltu", SLTU});
  add({"xor", XOR});
  add({"srl", SRL});
  add({"sra", SRA});
  add({"or", OR});
  add({"and", AND});
  add({"fence", FENCE});
  add({"fence.tso", FENCE_TSO});
  add({"ecall", ECALL});
  add({"ebreak", EBREAK});
}

static void add_rv32i_psueodo_instructions(riscv::MnemonicSet &mn_set) {
  using namespace riscv;
  auto add = [&](riscv::Mnemonic mn) { mn_set.insert(mn); };
  // with .option pic
  // load address: la rd, symbol -> auipc rd, symbol[31:12]; addi rd, rd, symbol[11:0]
  // else
  // load address: la rd, symbol -> auipc rd, symbol@GOT[31:12]; l{w|d} rd, symbol@GOT[11:0](rd)
  // load local address: lla rd, symbol -> auipc rd, symbol[31:12]; addi rd, rd, symbol[11:0]
  add({"j", J});
  add({"jr", JR});
  add({"ret", RET});
  add({"pause", PAUSE});
  add({"nop", NOP});
  add({"mv", MOVE});
  add({
      "not",
      NOT,
  });
  add({"neg", NEGATE});
  add({"sext.b", SEXT_B});
  add({"sext.h", SEXT_H});
  add({"zext.b", ZEXT_B});
  add({"zext.h", ZEXT_H});
  add({"seqz", SEQZ});
  add({"snez", SNEZ});
  add({"sltz", SLTZ});
  add({"sgtz", SGTZ});
  add({"beqz", BEQZ});
  add({"bnez", BNEZ});
  add({"blez", BLEZ});
  add({"bgez", BGEZ});
  add({"bltz", BLTZ});
  add({"bgtz", BGTZ});
  add({"bgt", BGT});
  add({"ble", BLE});
  add({"bgtu", BGTU});
  add({"bleu", BLEU});
}

static void add_rv32i(riscv::MnemonicSet &mn_set) {
  add_rv32i_instructions(mn_set);
  add_rv32i_psueodo_instructions(mn_set);
}

static auto mnemonics() {
  riscv::MnemonicSet mn_set;
  add_rv32i(mn_set);
  return mn_set;
}

const riscv::MnemonicSet riscv::string_to_mnemonic = mnemonics();
