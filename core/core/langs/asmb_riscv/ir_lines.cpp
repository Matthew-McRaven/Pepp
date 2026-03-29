#include "core/langs/asmb_riscv/ir_lines.hpp"

pepp::tc::IntegerInstruction::IntegerInstruction(riscv::MnemonicDescriptor desc) : mnemonic({desc}) {}

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

std::optional<u16> pepp::tc::IntegerInstruction::object_size(u16 base_address) const { return 4; }

pepp::tc::RTypeIR::RTypeIR(riscv::MnemonicDescriptor desc, u8 rd, u8 rs1, u8 rs2) : IntegerInstruction(desc) {
  this->rd = rd;
  this->rs1 = rs1;
  this->rs2 = rs2;
}
int pepp::tc::RTypeIR::type() const { return TYPE; }
pepp::tc::ITypeIR::ITypeIR(riscv::MnemonicDescriptor desc, u8 rd, u8 rs1, std::shared_ptr<ast::IRValue> imm)
    : IntegerInstruction(desc) {
  this->rd = rd;
  this->rs1 = rs1;
  this->imm = imm;
}

int pepp::tc::ITypeIR::type() const { return TYPE; }
pepp::tc::STypeIR::STypeIR(riscv::MnemonicDescriptor desc, u8 rs1, u8 rs2, std::shared_ptr<ast::IRValue> imm)
    : IntegerInstruction(desc) {
  this->rs1 = rs1;
  this->rs2 = rs2;
  this->imm = imm;
}

int pepp::tc::STypeIR::type() const { return TYPE; }
int pepp::tc::BTypeIR::type() const { return TYPE; }
int pepp::tc::UTypeIR::type() const { return TYPE; }
int pepp::tc::JTypeIR::type() const { return TYPE; }
