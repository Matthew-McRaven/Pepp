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
  // Bit 10 of imm field is used to encode shift type.
  ret.set_imm((shift_type & 0x1) << 10);
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::IFence(u8 fmt) {
  MnemonicDescriptor ret(Type::I, RV32I_FENCE);
  ret._funct3 = 0;
  // fmt is high-order 4 bits of imm
  ret.set_imm((fmt & 0b1111) << 8);
  return ret;
}

riscv::MnemonicDescriptor riscv::MnemonicDescriptor::IFence(u8 fmt, u8 pred, u8 succ) {
  MnemonicDescriptor ret(Type::I, RV32I_FENCE);
  ret._funct3 = 0;
  // fmt is high-order 4 bits of imm, pred middle 4 bits, and succ low-order 4 bits.
  ret.set_imm((fmt & 0b1111) << 8 | (pred & 0b1111) << 4 | (succ & 0b1111));
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

u8 riscv::MnemonicDescriptor::opcode() const {
  return _type == Type::Pseudo ? throw std::runtime_error("Pseudo instructions do not have a single opcode") : _opcode7;
}

bool riscv::MnemonicDescriptor::comma_after(std::size_t index) const noexcept {
  if (index >= _trailing_comma.size()) return false;
  return _trailing_comma[index];
}

void riscv::MnemonicDescriptor::append_operand(Operand operand) {
  for (int it = 0; it < _operands.size(); it++) {
    if (_operands[it].type == Operand::Type::Invalid) {
      _operands[it] = operand;
      return;
    }
  }
  throw std::runtime_error("Too many operands for this mnemonic");
}

riscv::MnemonicDescriptor &&riscv::MnemonicDescriptor::with_comma_after(std::size_t index, bool comma) && {
  if (index < _trailing_comma.size()) _trailing_comma[index] = comma;
  return std::move(*this);
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
  case Type::I: [[fallthrough]];
  case Type::S: [[fallthrough]];
  case Type::B: [[fallthrough]];
  case Type::U: [[fallthrough]];
  case Type::J: break;
  case Type::R: [[fallthrough]];
  case Type::INVALID: [[fallthrough]];
  case Type::Pseudo: [[fallthrough]];
  default: throw std::runtime_error("This mnemonic does not allow an immediate");
  }
  _imm_or_funct7 = imm;
  _flags.imm = 1;
}

std::optional<u32> riscv::MnemonicDescriptor::get_raw_imm() const {
  return has_imm() ? std::optional<u32>(_imm_or_funct7) : std::nullopt;
}

std::optional<u32> riscv::MnemonicDescriptor::get_shifted_imm() const {
  return has_imm() ? std::optional<u32>(encode_imm(_imm_or_funct7)) : std::nullopt;
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
  return 0;
}

// How far an operand's value is shifted down to reach the stored field. Branch and jump targets
// are always even, so bit 0 is implicit and not encoded. Everything else stores what it is given:
// `lui rd, 0x65` denotes the field 0x65, not the 0x65000 it eventually produces.
u8 riscv::MnemonicDescriptor::imm_shift() const noexcept {
  switch (_type) {
  case Type::B: [[fallthrough]];
  case Type::J: return 1;
  case Type::I: [[fallthrough]];
  case Type::S: [[fallthrough]];
  case Type::U: return 0;
  case Type::R: [[fallthrough]];
  case Type::INVALID: [[fallthrough]];
  case Type::Pseudo: return 0;
  }
  return 0;
}

// Narrow the range of imm to the encodable range of the instruction.
// Masking + shift is lossy for out-of-range value which can map to the opposite sign (beq +4096 encodes as
// -4096), and the shift drops low-order bits of the immediate for branches.
u32 riscv::MnemonicDescriptor::encode_imm(u32 imm) const noexcept {
  const auto width = width_imm();
  if (width == 0) return 0;
  return (imm >> imm_shift()) & ((u32(1) << width) - 1);
}

// Per ISA spec, sign-extended is required for most immediates.
// U-type is the exception, which is treated as unsigned.
bool riscv::MnemonicDescriptor::imm_fits(i32 imm) const noexcept {
  if (const int width = width_imm(); width == 0) return imm == 0;
  // Low-order bits are dropped if immediates are shifted.
  else if (const int shift = imm_shift(); imm & ((1 << shift) - 1)) return false;
  else if (u32 uimm = imm; _type == Type::U) return uimm <= (i32(1) << width) - 1;
  else {
    // Leading 0/1s are ignored when encoding.
    const int bits = width + shift;
    return -(i32(1) << (bits - 1)) <= imm && imm <= (i32(1) << (bits - 1)) - 1;
  }
}

bool riscv::MnemonicDescriptor::sources(Operand::Destination destination) const noexcept {
  for (const auto &operand : operands())
    if (operand.destination == destination) return true;
  return false;
}

u8 riscv::MnemonicDescriptor::resolve_rd(std::optional<u8> from_source) const noexcept {
  return (sources(Operand::Destination::RD) ? from_source.value_or(_rd) : _rd) & 0x1F;
}

u8 riscv::MnemonicDescriptor::resolve_rs1(std::optional<u8> from_source) const noexcept {
  // RS is the lone source of a two-operand pseudo and occupies the rs1 position.
  const bool written = sources(Operand::Destination::RS1) || sources(Operand::Destination::RS);
  return (written ? from_source.value_or(_rs1) : _rs1) & 0x1F;
}

u8 riscv::MnemonicDescriptor::resolve_rs2(std::optional<u8> from_source) const noexcept {
  return (sources(Operand::Destination::RS2) ? from_source.value_or(_rs2) : _rs2) & 0x1F;
}

bool riscv::MnemonicDescriptor::operator==(const MnemonicDescriptor &other) const noexcept {
  if (_type != other._type) return false;
  else if (_flags != other._flags) return false;
  else if (_opcode7 != other._opcode7 || _funct3 != other._funct3 || _imm_or_funct7 != other._imm_or_funct7 ||
           _rs1 != other._rs1 || _rs2 != other._rs2 || _rd != other._rd)
    return false;
  for (size_t i = 0; i < _operands.size(); i++)
    if (_operands[i].type != other._operands[i].type || _operands[i].destination != other._operands[i].destination)
      return false;
  return true;
}

riscv::rv_instruction2 riscv::MnemonicDescriptor::encode(Values v) const {
  switch (_type) {
  case Type::INVALID: return riscv::rv_instruction2(0u);
  case Type::R: return rv_instruction2(encode<InstructionR>(v));
  case Type::I: return rv_instruction2(encode<InstructionI>(v));
  case Type::S: return rv_instruction2(encode<InstructionS>(v));
  case Type::B: return rv_instruction2(encode<InstructionB>(v));
  case Type::U: return rv_instruction2(encode<InstructionU>(v));
  case Type::J: return rv_instruction2(encode<InstructionJ>(v));
  case Type::Pseudo: return riscv::rv_instruction2(0u);
  }
  return riscv::rv_instruction2(0u);
}

riscv::MnemonicDescriptor &&riscv::MnemonicDescriptor::with_imm(u32 imm) && {
  set_imm(imm);
  return std::move(*this);
}

riscv::MnemonicDescriptor::MnemonicDescriptor(Type type) : _type(type) {
  static const Operand invalid{.type = Operand::Type::Invalid, .destination = Operand::Destination::Invalid};
  _operands.fill(invalid);
}

riscv::MnemonicDescriptor::MnemonicDescriptor(Type type, u8 opcode) : _type(type), _opcode7(opcode) {
  static const Operand invalid{.type = Operand::Type::Invalid, .destination = Operand::Destination::Invalid};
  _operands.fill(invalid);
}

template <> riscv::InstructionR riscv::MnemonicDescriptor::encode<riscv::InstructionR>(Values v) const {
  const u8 rs1 = resolve_rs1(v.rs1);
  const u8 rs2 = resolve_rs2(v.rs2);
  const u8 rd = resolve_rd(v.rd);
  const u8 funct7 = _imm_or_funct7 & 0x7F;
  return InstructionR{.opcode = _opcode7, .rd = rd, .funct3 = _funct3, .rs1 = rs1, .rs2 = rs2, .funct7 = funct7};
}
template <> riscv::InstructionI riscv::MnemonicDescriptor::encode<riscv::InstructionI>(Values v) const {
  const u8 rs1 = resolve_rs1(v.rs1);
  const u8 rd = resolve_rd(v.rd);
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u16 imm = encode_imm(v.imm.value_or(0) | _imm_or_funct7);
  return InstructionI{.opcode = _opcode7, .rd = rd, .funct3 = _funct3, .rs1 = rs1, .imm = imm};
}
template <> riscv::InstructionS riscv::MnemonicDescriptor::encode<riscv::InstructionS>(Values v) const {
  const u8 rs1 = resolve_rs1(v.rs1);
  const u8 rs2 = resolve_rs2(v.rs2);
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u16 imm = encode_imm(v.imm.value_or(0) | _imm_or_funct7);
  const u8 imm11_05 = (imm >> 5u) & ((1 << 7) - 1);
  const u8 imm4_0 = imm & ((1 << 5) - 1);
  return InstructionS{.opcode = _opcode7, .imm1 = imm4_0, .funct3 = _funct3, .rs1 = rs1, .rs2 = rs2, .imm2 = imm11_05};
}
template <> riscv::InstructionU riscv::MnemonicDescriptor::encode<riscv::InstructionU>(Values v) const {
  const u8 rd = resolve_rd(v.rd);
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u32 imm = encode_imm(v.imm.value_or(0) | _imm_or_funct7);
  return InstructionU{.opcode = _opcode7, .rd = rd, .imm = imm};
}
template <> riscv::InstructionB riscv::MnemonicDescriptor::encode<riscv::InstructionB>(Values v) const {
  const u8 rs1 = resolve_rs1(v.rs1);
  const u8 rs2 = resolve_rs2(v.rs2);
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u32 imm = encode_imm(v.imm.value_or(0) | _imm_or_funct7);
  const u8 imm4_1 = imm & ((1u << 4) - 1);
  const u8 imm10_5 = (imm >> 4u) & ((1u << 6) - 1);
  const u8 imm11_11 = (imm >> 10u) & 1;
  const u8 imm12_12 = (imm >> 11u) & 1;
  return InstructionB{.opcode = _opcode7,
                      .imm1 = imm11_11,
                      .imm2 = imm4_1,
                      .funct3 = _funct3,
                      .rs1 = rs1,
                      .rs2 = rs2,
                      .imm3 = imm10_5,
                      .imm4 = imm12_12};
}
template <> riscv::InstructionJ riscv::MnemonicDescriptor::encode<riscv::InstructionJ>(Values v) const {
  const u8 rd = resolve_rd(v.rd);
  // Sometime immediate already has bits in it for specialized instructions. Preserve those bits with |
  const u32 imm = encode_imm(v.imm.value_or(0) | _imm_or_funct7);
  const u16 imm10_01 = imm & ((1u << 10) - 1);
  const u8 imm11_11 = (imm >> 10u) & 1;
  const u16 imm19_12 = (imm >> 11u) & ((1u << 8) - 1);
  const u8 imm20_20 = (imm >> 19u) & 1;
  return InstructionJ{
      .opcode = _opcode7, .rd = rd, .imm1 = imm19_12, .imm2 = imm11_11, .imm3 = imm10_01, .imm4 = imm20_20};
}

static void add_rv32i_instructions(riscv::MnemonicSet &mn_set) {
  using namespace riscv;
  // The spelling comes from RV_OP_INFO, the same table the disassembler formats out of, so the
  // two can no longer disagree. Each line below pairs an op with the descriptor that encodes it,
  // which is the link that previously existed only by the two identifiers being spelled alike.
  auto add = [&](RvOp op, const MnemonicDescriptor &desc) {
    mn_set.insert(Mnemonic{std::string(mnemonic(op)), desc});
  };
  add(RvOp::LUI, LUI);
  add(RvOp::AUIPC, AUIPC);
  add(RvOp::JAL, JAL_nord);
  add(RvOp::JAL, JAL);
  add(RvOp::JALR, JALR);
  add(RvOp::BEQ, BEQ);
  add(RvOp::BNE, BNE);
  add(RvOp::BLT, BLT);
  add(RvOp::BGE, BGE);
  add(RvOp::BLTU, BLTU);
  add(RvOp::BGEU, BGEU);
  add(RvOp::LB, LB);
  add(RvOp::LH, LH);
  add(RvOp::LW, LW);
  add(RvOp::LBU, LBU);
  add(RvOp::LHU, LHU);
  add(RvOp::SB, SB);
  add(RvOp::SH, SH);
  add(RvOp::SW, SW);
  add(RvOp::ADDI, ADDI);
  add(RvOp::SLTI, SLTI);
  add(RvOp::SLTIU, SLTIU);
  add(RvOp::XORI, XORI);
  add(RvOp::ORI, ORI);
  add(RvOp::ANDI, ANDI);
  add(RvOp::SLLI, SLLI);
  add(RvOp::SRLI, SRLI);
  add(RvOp::SRAI, SRAI);
  add(RvOp::ADD, ADD);
  add(RvOp::SUB, SUB);
  add(RvOp::SLL, SLL);
  add(RvOp::SLT, SLT);
  add(RvOp::SLTU, SLTU);
  add(RvOp::XOR, XOR);
  add(RvOp::SRL, SRL);
  add(RvOp::SRA, SRA);
  add(RvOp::OR, OR);
  add(RvOp::AND, AND);
  add(RvOp::FENCE, FENCE);
  add(RvOp::FENCE_TSO, FENCE_TSO);
  add(RvOp::ECALL, ECALL);
  add(RvOp::EBREAK, EBREAK);
}

static void add_rv32i_psueodo_instructions(riscv::MnemonicSet &mn_set) {
  using namespace riscv;
  auto add = [&](const riscv::Mnemonic &mn) { mn_set.insert(mn); };
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
  add({"not", NOT});
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
