#include "core/arch/riscv/asmb/rv_mnemonics.hpp"
#include "core/arch/riscv/asmb/rvi_patterns.hpp"
#include "core/arch/riscv/isa/rv_instruction_list.hpp"

riscv::MnemonicU::MnemonicU(uint8_t opcode7) noexcept : _opcode7(opcode7 & 0x7F) {}

bool riscv::MnemonicU::has_immediate() const noexcept { return _flags.imm; }

void riscv::MnemonicU::set_immediate(uint32_t imm) {
  _imm20 = (imm >> 12u) & (1 << 20) - 1;
  _flags.imm = 1;
}

std::optional<uint32_t> riscv::MnemonicU::get_immediate() const {
  return _flags.imm ? std::optional<uint32_t>(_imm20 << 12u) : std::nullopt;
}

riscv::MnemonicU &&riscv::MnemonicU::with_immediate(uint32_t imm) && {
  set_immediate(imm);
  return std::move(*this);
}

bool riscv::MnemonicU::has_rd() const noexcept { return _flags.rd; }

void riscv::MnemonicU::set_rd(uint8_t rd) {
  _rd5 = rd & (1 << 5) - 1;
  _flags.rd = 1;
}

std::optional<uint8_t> riscv::MnemonicU::get_rd() const {
  return _flags.rd ? std::optional<uint8_t>(_rd5) : std::nullopt;
}

riscv::MnemonicU &&riscv::MnemonicU::with_rd(uint8_t rd) && {
  set_rd(rd);
  return std::move(*this);
}

riscv::InstructionU riscv::MnemonicU::to_instruction() const noexcept {
  // Prefer to return instruction even if some fields are missing. They will be initialized to 0.
  return InstructionU{.opcode = _opcode7, .rd = _rd5, .imm = _imm20};
}

riscv::MnemonicJ::MnemonicJ(uint8_t opcode7) noexcept : _opcode7(opcode7 & 0x7F) {}

bool riscv::MnemonicJ::has_immediate() const noexcept { return _flags.imm; }

void riscv::MnemonicJ::set_immediate(uint32_t imm) {
  // Keep only bits [20:1], discarding [0], because we cannot jump to odd addresses.
  _imm20 = (imm >> 1u) & ((1 << 20) - 1);
  _flags.imm = 1;
}

std::optional<uint32_t> riscv::MnemonicJ::get_immediate() const {
  if (!_flags.imm) return std::nullopt;
  return _imm20 << 1u;
}

riscv::MnemonicJ &&riscv::MnemonicJ::with_immediate(uint32_t imm) && {
  set_immediate(imm);
  return std::move(*this);
}

bool riscv::MnemonicJ::has_rd() const noexcept { return _flags.rd; }

void riscv::MnemonicJ::set_rd(uint8_t rd) {
  _rd5 = rd & (1 << 5) - 1;
  _flags.rd = 1;
}

std::optional<uint8_t> riscv::MnemonicJ::get_rd() const {
  return _flags.rd ? std::optional<uint8_t>(_rd5) : std::nullopt;
}

riscv::MnemonicJ &&riscv::MnemonicJ::with_rd(uint8_t rd) && {
  set_rd(rd);
  return std::move(*this);
}
riscv::InstructionJ riscv::MnemonicJ::to_instruction() const noexcept {
  const uint8_t imm20_20 = (_imm20 >> 20u) & 1;
  const uint16_t imm10_01 = (_imm20 >> 1u) & ((1 << 10) - 1);
  const uint8_t imm11_11 = (_imm20 >> 11u) & 1;
  const uint16_t imm19_12 = (_imm20 >> 12u) & ((1 << 8) - 1);
  return InstructionJ{
      .opcode = _opcode7, .rd = _rd5, .imm1 = imm19_12, .imm2 = imm20_20, .imm3 = imm10_01, .imm4 = imm11_11};
}

riscv::MnemonicB::MnemonicB(uint8_t opcode7, uint8_t funct3) noexcept
    : _opcode7(opcode7 & 0x7F), _funct3(funct3 & 0x7) {}

bool riscv::MnemonicB::has_immediate() const noexcept { return _flags.imm; }

void riscv::MnemonicB::set_immediate(uint16_t imm) {
  _imm12 = (imm >> 1u) & ((1 << 12) - 1);
  _flags.imm = 1;
}

std::optional<uint16_t> riscv::MnemonicB::get_immediate() const {
  return _flags.imm ? std::optional<uint16_t>(_imm12 << 1u) : std::nullopt;
}

riscv::MnemonicB &&riscv::MnemonicB::with_immediate(uint16_t imm) && {
  set_immediate(imm);
  return std::move(*this);
}

bool riscv::MnemonicB::has_rs1() const noexcept { return _flags.rs1; }

void riscv::MnemonicB::set_rs1(uint8_t rs1) {
  _rs15 = rs1 & (1 << 5) - 1;
  _flags.rs1 = 1;
}

std::optional<uint8_t> riscv::MnemonicB::get_rs1() const {
  return _flags.rs1 ? std::optional<uint8_t>(_rs15) : std::nullopt;
}

riscv::MnemonicB &&riscv::MnemonicB::with_rs1(uint8_t rs1) && {
  set_rs1(rs1);
  return std::move(*this);
}

bool riscv::MnemonicB::has_rs2() const noexcept { return _flags.rs2; }

void riscv::MnemonicB::set_rs2(uint8_t rs2) {
  _rs25 = rs2 & (1 << 5) - 1;
  _flags.rs2 = 1;
}

std::optional<uint8_t> riscv::MnemonicB::get_rs2() const {
  return _flags.rs2 ? std::optional<uint8_t>(_rs25) : std::nullopt;
}

riscv::MnemonicB &&riscv::MnemonicB::with_rs2(uint8_t rs2) && {
  set_rs2(rs2);
  return std::move(*this);
}

riscv::InstructionB riscv::MnemonicB::to_instruction() const noexcept {
  const uint8_t imm12_12 = (_imm12 >> 12u) & 1;
  const uint8_t imm10_5 = (_imm12 >> 5u) & ((1 << 6) - 1);
  const uint8_t imm4_1 = (_imm12 >> 1u) & ((1 << 4) - 1);
  const uint8_t imm11_11 = (_imm12 >> 11u) & 1;
  return InstructionB{.opcode = _opcode7,
                      .imm1 = imm4_1,
                      .imm2 = imm10_5,
                      .funct3 = _funct3,
                      .rs1 = _rs15,
                      .rs2 = _rs25,
                      .imm3 = imm11_11,
                      .imm4 = imm12_12};
}

riscv::MnemonicI::MnemonicI(uint8_t opcode7, uint8_t funct3) noexcept
    : _opcode7(opcode7 & 0x7F), _funct3(funct3 & 0x7) {}

bool riscv::MnemonicI::has_immediate() const noexcept { return _flags.imm; }

void riscv::MnemonicI::set_immediate(uint16_t imm) {
  _imm12 = imm & ((1 << 12) - 1);
  _flags.imm = 1;
}

std::optional<uint16_t> riscv::MnemonicI::get_immediate() const {
  return _flags.imm ? std::optional<uint16_t>(_imm12) : std::nullopt;
}

riscv::MnemonicI &&riscv::MnemonicI::with_immediate(uint16_t imm) && {
  set_immediate(imm);
  return std::move(*this);
}

bool riscv::MnemonicI::has_rs1() const noexcept { return _flags.rs1; }

void riscv::MnemonicI::set_rs1(uint8_t rs1) {
  _rs15 = rs1 & (1 << 5) - 1;
  _flags.rs1 = 1;
}

std::optional<uint8_t> riscv::MnemonicI::get_rs1() const {
  return _flags.rs1 ? std::optional<uint8_t>(_rs15) : std::nullopt;
}

riscv::MnemonicI &&riscv::MnemonicI::with_rs1(uint8_t rs1) && {
  set_rs1(rs1);
  return std::move(*this);
}

bool riscv::MnemonicI::has_rd() const noexcept { return _flags.rd; }

void riscv::MnemonicI::set_rd(uint8_t rd) {
  _rd5 = rd & (1 << 5) - 1;
  _flags.rd = 1;
}

std::optional<uint8_t> riscv::MnemonicI::get_rd() const {
  return _flags.rd ? std::optional<uint8_t>(_rd5) : std::nullopt;
}

riscv::MnemonicI &&riscv::MnemonicI::with_rd(uint8_t rd) && {
  set_rd(rd);
  return std::move(*this);
}

riscv::InstructionI riscv::MnemonicI::to_instruction() const noexcept {
  return InstructionI{.opcode = _opcode7, .rd = _rd5, .funct3 = _funct3, .rs1 = _rs15, .imm = _imm12};
}

riscv::ConstantShiftMnemonic::ConstantShiftMnemonic(uint8_t opcode7, uint8_t funct3) noexcept
    : MnemonicI(opcode7, funct3) {}

void riscv::ConstantShiftMnemonic::set_shift_type(uint8_t shift_type) {
  // Mask out bit [10] of the immediate, and then set it to shift_type's bit [0].
  _imm12 = (_imm12 & ~(1 << 10)) | ((shift_type & 1) << 10);
  _flags.imm = 1;
}

std::optional<uint8_t> riscv::ConstantShiftMnemonic::get_shift_type() const {
  return _flags.imm ? std::optional<uint8_t>((_imm12 >> 10) & 1) : std::nullopt;
}

riscv::ConstantShiftMnemonic &&riscv::ConstantShiftMnemonic::with_shift_type(uint8_t shift_type) && {
  set_shift_type(shift_type);
  return std::move(*this);
}

void riscv::ConstantShiftMnemonic::set_shamt(uint8_t shamt) {
  // Encoded in lower order 5 bits of imm. Mask out existing value and replace
  _imm12 = (_imm12 & ~((1 << 5) - 1)) | (shamt & ((1 << 5) - 1));
  _flags.imm = 1;
}

std::optional<uint8_t> riscv::ConstantShiftMnemonic::get_shamt() const {
  return _flags.imm ? std::optional<uint8_t>(_imm12 & ((1 << 5) - 1)) : std::nullopt;
}

riscv::ConstantShiftMnemonic &&riscv::ConstantShiftMnemonic::with_shamt(uint8_t shamt) && {
  set_shamt(shamt);
  return std::move(*this);
}

riscv::FenceFormat::FenceFormat() noexcept : MnemonicI(RV32I_FENCE, 0b000) {
  set_rd(0);
  set_rs1(0);
}

static uint32_t FM_SHIFT = 8u;
static uint32_t FM_MASK = 0b1111 << FM_SHIFT;
static uint32_t PRED_SHIFT = 4u;
static uint32_t IM_PRED_MASK = 0b1111 << PRED_SHIFT;
void riscv::FenceFormat::set_fm(uint8_t fm) {
  // mask out bits beyond [3:0] of fm
  fm = fm & 0b1111;
  // Clear and set high-order 4 bits of imm.
  _imm12 = (_imm12 & FM_MASK) | (fm << FM_SHIFT);
  _flags.imm = 1;
}

std::optional<uint8_t> riscv::FenceFormat::get_fm() const {
  return _flags.imm ? std::optional<uint8_t>((_imm12 >> FM_SHIFT) & 0b1111) : std::nullopt;
}

riscv::FenceFormat &&riscv::FenceFormat::with_fm(uint8_t fm) && {
  set_fm(fm);
  return std::move(*this);
}

void riscv::FenceFormat::set_pred(uint8_t pred) {
  // mask out bits beyond [3:0] of pred
  pred = pred & 0b1111;
  // Clear and set middle 4 bits of imm.
  _imm12 = (_imm12 & ~IM_PRED_MASK) | (pred << PRED_SHIFT);
  _flags.imm = 1;
}

std::optional<uint8_t> riscv::FenceFormat::get_pred() const {
  return _flags.imm ? std::optional<uint8_t>((_imm12 >> PRED_SHIFT) & 0b1111) : std::nullopt;
}

riscv::FenceFormat &&riscv::FenceFormat::with_pred(uint8_t pred) && {
  set_pred(pred);
  return std::move(*this);
}

void riscv::FenceFormat::set_succ(uint8_t succ) {
  // mask out bits beyond [3:0] of succ
  succ = succ & 0b1111;
  // Clear and set low-order 4 bits of imm.
  _imm12 = (_imm12 & ~0b1111) | succ;
  _flags.imm = 1;
}

std::optional<uint8_t> riscv::FenceFormat::get_succ() const {
  return _flags.imm ? std::optional<uint8_t>(_imm12 & 0b1111) : std::nullopt;
}

riscv::FenceFormat &&riscv::FenceFormat::with_succ(uint8_t succ) && {
  set_succ(succ);
  return std::move(*this);
}

const uint8_t riscv::FenceFormat::merge_iorw(bool i, bool o, bool r, bool w) noexcept {
  return (i << 3u) | (o << 2u) | (r << 1u) | w;
}

riscv::MnemonicR::MnemonicR(uint8_t opcode7, uint8_t funct3, uint8_t funct7) noexcept
    : _opcode7(opcode7 & 0x7f), _funct3(funct3 & 0x7), _funct7(funct7 & 0x7f) {}

bool riscv::MnemonicR::has_rs1() const noexcept { return _flags.rs1; }

void riscv::MnemonicR::set_rs1(uint8_t rs1) {
  _rs15 = rs1 & (1 << 5) - 1;
  _flags.rs1 = 1;
}

std::optional<uint8_t> riscv::MnemonicR::get_rs1() const {
  return _flags.rs1 ? std::optional<uint8_t>(_rs15) : std::nullopt;
}

riscv::MnemonicR &&riscv::MnemonicR::with_rs1(uint8_t rs1) && {
  set_rs1(rs1);
  return std::move(*this);
}

bool riscv::MnemonicR::has_rs2() const noexcept { return _flags.rs2; }

void riscv::MnemonicR::set_rs2(uint8_t rs2) {
  _rs25 = rs2 & (1 << 5) - 1;
  _flags.rs2 = 1;
}

std::optional<uint8_t> riscv::MnemonicR::get_rs2() const {
  return _flags.rs2 ? std::optional<uint8_t>(_rs25) : std::nullopt;
}

riscv::MnemonicR &&riscv::MnemonicR::with_rs2(uint8_t rs2) && {
  set_rs2(rs2);
  return std::move(*this);
}

bool riscv::MnemonicR::has_rd() const noexcept { return _flags.rd; }

void riscv::MnemonicR::set_rd(uint8_t rd) {
  _rd5 = rd & (1 << 5) - 1;
  _flags.rd = 1;
}

std::optional<uint8_t> riscv::MnemonicR::get_rd() const {
  return _flags.rd ? std::optional<uint8_t>(_rd5) : std::nullopt;
}

riscv::MnemonicR &&riscv::MnemonicR::with_rd(uint8_t rd) && {
  set_rd(rd);
  return std::move(*this);
}

riscv::InstructionR riscv::MnemonicR::to_instruction() const noexcept {
  return InstructionR{.opcode = _opcode7, .rd = _rd5, .funct3 = _funct3, .rs1 = _rs15, .rs2 = _rs25, .funct7 = _funct7};
}

riscv::MnemonicS::MnemonicS(uint8_t opcode7, uint8_t funct3) noexcept
    : _opcode7(opcode7 & 0x7f), _funct3(funct3 & 0x7) {}

bool riscv::MnemonicS::has_immediate() const noexcept { return _flags.imm; }

void riscv::MnemonicS::set_immediate(uint16_t imm) {
  _imm12 = imm & ((1 << 12) - 1);
  _flags.imm = 1;
}

std::optional<uint16_t> riscv::MnemonicS::get_immediate() const {
  return _flags.imm ? std::optional<uint16_t>(_imm12) : std::nullopt;
}

riscv::MnemonicS &&riscv::MnemonicS::with_immediate(uint16_t imm) && {
  set_immediate(imm);
  return std::move(*this);
}

bool riscv::MnemonicS::has_rs1() const noexcept { return _flags.rs1; }

void riscv::MnemonicS::set_rs1(uint8_t rs1) {
  _rs15 = rs1 & (1 << 5) - 1;
  _flags.rs1 = 1;
}

std::optional<uint8_t> riscv::MnemonicS::get_rs1() const {
  return _flags.rs1 ? std::optional<uint8_t>(_rs15) : std::nullopt;
}

riscv::MnemonicS &&riscv::MnemonicS::with_rs1(uint8_t rs1) && {
  set_rs1(rs1);
  return std::move(*this);
}

bool riscv::MnemonicS::has_rs2() const noexcept { return _flags.rs2; }

void riscv::MnemonicS::set_rs2(uint8_t rs2) {
  _rs25 = rs2 & (1 << 5) - 1;
  _flags.rs2 = 1;
}

std::optional<uint8_t> riscv::MnemonicS::get_rs2() const {
  return _flags.rs2 ? std::optional<uint8_t>(_rs25) : std::nullopt;
}

riscv::MnemonicS &&riscv::MnemonicS::with_rs2(uint8_t rs2) && {
  set_rs2(rs2);
  return std::move(*this);
}

riscv::InstructionS riscv::MnemonicS::to_instruction() const noexcept {
  const uint8_t imm11_05 = (_imm12 >> 5u) & ((1 << 7) - 1);
  const uint8_t imm4_0 = _imm12 & ((1 << 5) - 1);
  return InstructionS{
      .opcode = _opcode7, .imm1 = imm4_0, .funct3 = _funct3, .rs1 = _rs15, .rs2 = _rs25, .imm2 = imm11_05};
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
  add({"nop", NOP});
  // with .option pic
  // load address: la rd, symbol -> auipc rd, symbol[31:12]; addi rd, rd, symbol[11:0]
  // else
  // load address: la rd, symbol -> auipc rd, symbol@GOT[31:12]; l{w|d} rd, symbol@GOT[11:0](rd)
  // load local address: lla rd, symbol -> auipc rd, symbol[31:12]; addi rd, rd, symbol[11:0]
  // move: mv rd, rs -> addi rd, rs, 0
  // not: not rd, rs -> xori rd, rs, -1
  // negate: neg rd, rs -> sub rd, x0, rs
  // negate: negw rd, rs -> subw rd, x0, rs
  // sign extend byte: sext.b rd, rs  -> slli rd, rs, XLEN - 8; srai rd, rd, XLEN - 8
  // sign extend halfword: sext.h rd, rs -> slli rd, rs, XLEN - 16; srai rd, rd, XLEN - 16
  // zero extend byte: zext.b rd, rs -> andi rd, rs, 255
  // zero extend halfword: zext.h rd, rs -> slli rd, rs, XLEN - 16; srli rd, rd, XLEN - 16
  // set if equal to 0: seqz rd, rs -> sltiu rd, rs, 1
  // set if not equal to 0: snez rd, rs -> sltu rd, x0, rs
  // set if < 0: sltz rd, rs -> slt rd, rs, x0
  // set if > 0: sgtz rd, rs -> slt rd, x0, rs
  // branch if equal to 0: beqz rs, offset -> beq rs, x0, offset
  // branch if not equal to 0: bnez rs, offset -> bne rs, x0, offset
  // branch if <= 0: blez rs, offset -> bge x0, rs, offset
  // branch if >= 0: bgez rs, offset -> bge rs, x0, offset
  // branch if < 0: bltz rs, offset -> blt rs, x0, offset
  // branch if > 0: bgtz rs, offset -> blt x0, rs, offset
  // branch if >: bgt rs1, rs2, offset -> blt rs2, rs1, offset
  // branch if <=: ble rs1, rs2, offset -> bge rs2, rs1, offset
  // branch if > unsigned: bgtu rs1, rs2, offset -> bltu rs2, rs1, offset
  // branch if <= unsigned: bleu rs1, rs2, offset -> bgeu rs2, rs1, offset
  add({"j", J});
  add({"jr", JR});
  add({"ret", RET});
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
