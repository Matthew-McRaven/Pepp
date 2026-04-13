#pragma once

#include "core/arch/riscv/asmb/rv_mnemonics.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/langs/asmb_riscv/ir_attributes.hpp"

namespace pepp::ast {
class IRValue;
}

namespace pepp::tc {
enum class RISCVDotCommands : int {
  ASCIZ = static_cast<int>(DotCommands::FIRST_USER),
};
enum class RISCVIRType : int { R = static_cast<int>(LinearIRType::FirstUser), I, S, B, U, J };
struct IntegerInstruction : public LinearIR {
  IntegerInstruction(riscv::MnemonicDescriptor desc);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u16> object_size(u16 base_address) const override;

  RISCVMnemonicAttribute mnemonic;
  u8 rd, rs1, rs2;
  std::shared_ptr<pepp::ast::IRValue> imm;
};

struct RTypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::R);
  RTypeIR(riscv::MnemonicDescriptor desc, u8 rd, u8 rs1, u8 rs2);
  virtual ~RTypeIR() override = default;
  int type() const override;
};

struct ITypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::I);
  ITypeIR(riscv::MnemonicDescriptor desc, u8 rd, u8 rs1, std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

struct STypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::S);
  STypeIR(riscv::MnemonicDescriptor desc, u8 rs1, u8 rs2, std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

struct BTypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::B);
  BTypeIR(riscv::MnemonicDescriptor desc, u8 rs1, u8 rs2, std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

struct UTypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::U);
  UTypeIR(riscv::MnemonicDescriptor desc, u8 rd, std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

struct JTypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::J);
  JTypeIR(riscv::MnemonicDescriptor desc, u8 rd, std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};
} // namespace pepp::tc
