#include "core/langs/asmb_riscv/ir_lines.hpp"
#include "core/compile/ir_value/numeric.hpp"

pepp::tc::IntegerInstruction::IntegerInstruction(std::string_view name, riscv::MnemonicDescriptor desc)
    : mnemonic({name, desc}) {}

const pepp::tc::AAttribute *pepp::tc::IntegerInstruction::attribute(int type) const {
  if (type == RISCVMnemonicAttribute::TYPE) return &mnemonic;
  else return LinearIR::attribute(type);
}

void pepp::tc::IntegerInstruction::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == RISCVMnemonicAttribute::TYPE) {
    auto as_mnemonic = static_cast<RISCVMnemonicAttribute *>(attr.get());
    mnemonic = *as_mnemonic;
  } else LinearIR::insert(std::move(attr));
}

std::optional<u64> pepp::tc::IntegerInstruction::object_size(u64 base_address) const { return 4; }

pepp::tc::RTypeIR::RTypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rd, u8 rs1, u8 rs2)
    : IntegerInstruction(name, desc) {
  this->rd = rd;
  this->rs1 = rs1;
  this->rs2 = rs2;
}
int pepp::tc::RTypeIR::type() const { return TYPE; }
pepp::tc::ITypeIR::ITypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rd, u8 rs1,
                           std::shared_ptr<ast::IRValue> imm)
    : IntegerInstruction(name, desc) {
  this->rd = rd;
  this->rs1 = rs1;
  this->imm = imm;
}

int pepp::tc::ITypeIR::type() const { return TYPE; }
pepp::tc::STypeIR::STypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rs1, u8 rs2,
                           std::shared_ptr<ast::IRValue> imm)
    : IntegerInstruction(name, desc) {
  this->rs1 = rs1;
  this->rs2 = rs2;
  this->imm = imm;
}

int pepp::tc::STypeIR::type() const { return TYPE; }
pepp::tc::BTypeIR::BTypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rs1, u8 rs2,
                           std::shared_ptr<ast::IRValue> imm)
    : IntegerInstruction(name, desc) {
  this->rs1 = rs1;
  this->rs2 = rs2;
  this->imm = imm;
}

int pepp::tc::BTypeIR::type() const { return TYPE; }

pepp::tc::UTypeIR::UTypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rd,
                           std::shared_ptr<ast::IRValue> imm)
    : IntegerInstruction(name, desc) {
  this->rd = rd;
  this->imm = imm;
}

int pepp::tc::UTypeIR::type() const { return TYPE; }

int pepp::tc::JTypeIR::type() const { return TYPE; }

pepp::tc::JTypeIR::JTypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rd,
                           std::shared_ptr<ast::IRValue> imm)
    : IntegerInstruction(name, desc) {
  this->rd = rd;
  this->imm = imm;
}

pepp::tc::DotSymbol::DotSymbol(Which which, Argument arg) : which(which), argument(arg) {}

const pepp::tc::AAttribute *pepp::tc::DotSymbol::attribute(int type) const {
  if (type == Argument::TYPE) return &argument;
  else return LinearIR::attribute(type);
}

void pepp::tc::DotSymbol::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Argument::TYPE) argument = *(static_cast<Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

int pepp::tc::DotSymbol::type() const { return TYPE; }

std::shared_ptr<pepp::tc::IntegerInstruction> pepp::tc::make_instruction(std::string_view name,
                                                                        const riscv::MnemonicDescriptor &desc,
                                                                        const ParsedOperands &operands) {
  using Type = riscv::MnemonicDescriptor::Type;
  switch (desc.type()) {
  case Type::R: return std::make_shared<RTypeIR>(name, desc, operands.rd, operands.rs1, operands.rs2);
  case Type::I: {
    // Fence stores orderings inside immediate. Create necessary imm value here.
    using Dest = riscv::Operand::Destination;
    if (desc.sources(Dest::PRED) || desc.sources(Dest::SUCC)) {
      const u32 order = (u32(operands.pred & 0xF) << 4) | u32(operands.succ & 0xF);
      // fmt bits are pre-set in high-order bits on MnemonicDescriptor
      const u32 fmt = desc.get_raw_imm().value_or(0) & 0xFF;
      const auto imm = std::make_shared<pepp::ast::UnsignedDecimal>(fmt | order, 2);
      return std::make_shared<ITypeIR>(name, desc, operands.rd, operands.rs1, imm);
    }
    return std::make_shared<ITypeIR>(name, desc, operands.rd, operands.rs1, operands.imm);
  }
  case Type::S: return std::make_shared<STypeIR>(name, desc, operands.rs1, operands.rs2, operands.imm);
  case Type::B: return std::make_shared<BTypeIR>(name, desc, operands.rs1, operands.rs2, operands.imm);
  case Type::U: return std::make_shared<UTypeIR>(name, desc, operands.rd, operands.imm);
  case Type::J: return std::make_shared<JTypeIR>(name, desc, operands.rd, operands.imm);
  case Type::Pseudo: [[fallthrough]];
  case Type::INVALID: return nullptr;
  }
  return nullptr;
}
