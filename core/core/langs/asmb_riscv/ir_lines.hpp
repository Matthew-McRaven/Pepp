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
  ALIGN_P2,
  ALIGN_BYTE,
  SYMBOL_GLOBAL,
  SYMBOL_LOCAL,
  SYMBOL_WEAK,
  SYMBOL_HIDDEN,
  SECTION_TEXT,
  SECTION_BSS,
  SECTION_RODATA,
  SECTION_DATA,
};
enum class RISCVIRType : int { R = static_cast<int>(LinearIRType::FirstUser), I, S, B, U, J };
struct IntegerInstruction : public LinearIR {
  IntegerInstruction(std::string_view name, riscv::MnemonicDescriptor desc);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u64> object_size(u64 base_address) const override;

  RISCVMnemonicAttribute mnemonic;
  u8 rd, rs1, rs2;
  std::shared_ptr<pepp::ast::IRValue> imm;
};

struct RTypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::R);
  RTypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rd, u8 rs1, u8 rs2);
  virtual ~RTypeIR() override = default;
  int type() const override;
};

struct ITypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::I);
  ITypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rd, u8 rs1,
          std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

struct STypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::S);
  STypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rs1, u8 rs2,
          std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

struct BTypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::B);
  BTypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rs1, u8 rs2,
          std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

struct UTypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::U);
  UTypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rd, std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

struct JTypeIR : public IntegerInstruction {
  static constexpr int TYPE = static_cast<int>(RISCVIRType::J);
  JTypeIR(std::string_view name, riscv::MnemonicDescriptor desc, u8 rd, std::shared_ptr<pepp::ast::IRValue> imm);
  int type() const override;
};

// Values of fields as parsed for an instruction. Some duplication w.r.t. riscv::Values, but this representation
// provides stronger typing for immediates.
struct ParsedOperands {
  u8 rd = 0, rs1 = 0, rs2 = 0;
  // Only used by fence instructions to avoid having to read-modify-write immediate.
  u8 pred = 0, succ = 0;
  std::shared_ptr<pepp::ast::IRValue> imm;
};

// Select the correct subclass based on MnemonicDescriptor.
std::shared_ptr<IntegerInstruction> make_instruction(std::string_view name, const riscv::MnemonicDescriptor &desc,
                                                     const ParsedOperands &operands);

struct DotSymbol : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotSymbol);
  enum class Which { Global, Local, Weak, Hidden } which;
  // Arg must always be an identifier
  DotSymbol(Which which, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  Argument argument;
};

} // namespace pepp::tc
