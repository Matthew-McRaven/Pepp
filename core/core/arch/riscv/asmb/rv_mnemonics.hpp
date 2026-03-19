#pragma once
#include <array>
#include <flat/flat_set.hpp>
#include <optional>
#include <span>
#include "core/arch/riscv/isa/rv_instruction.hpp"
#include "core/arch/riscv/isa/rvi.hpp"
#include "core/integers.h"

namespace riscv {

struct Values {
  std::optional<uint8_t> rs1, rs2, rd;
  std::optional<uint32_t> imm;
};

struct Operand {
  enum class Type : u8 { Invalid = 0, Register, Immediate, Fence, XLEN8, XLEN16 } type;
  enum class Destination : u8 { Invalid = 0, RS, RS1, RS2, RD, IMM, SHAMT, PRED, SUCC } destination;
};

struct MnemonicDescriptor {
  enum class Type : u8 { INVALID = 0, R, I, S, B, U, J, Pseudo };
  static MnemonicDescriptor I(u8 opcode7, u8 funct3);
  static MnemonicDescriptor IShiftByConstant(u8 opcode7, u8 funct3, u8 shift_type);
  static MnemonicDescriptor IFence(u8 fmt);
  static MnemonicDescriptor IFence(u8 fmt, u8 pred, u8 succ);
  static MnemonicDescriptor R(u8 opcode7, u8 funct3, u8 funct7);
  static MnemonicDescriptor S(u8 opcode7, u8 funct3);
  static MnemonicDescriptor B(u8 opcode7, u8 funct3);
  static MnemonicDescriptor U(u8 opcode7);
  static MnemonicDescriptor J(u8 opcode7);
  // Pseudo instrutions do not have a single encoding since they may expand to multiple instructions.
  // However, they still have operands that need parsing.
  // Pseudo-instructions that can be encoded in terms of a real mnemonic should prefer a non-pseudo encoding.
  static MnemonicDescriptor Pseudo();

  std::span<const Operand> operands() const noexcept;
  Type type() const noexcept;

  void append_operand(Operand operand);
  MnemonicDescriptor &&with_operand(Operand first, std::same_as<Operand> auto... ops) &&;
  MnemonicDescriptor replaced_operands(std::same_as<Operand> auto... ops) const noexcept;

  // Does this mnemonic have an rs1 position in its instruction format?
  bool allows_rs1() const noexcept;
  // Is the value of rs1 a constant specified by the instruction?
  bool has_rs1() const noexcept;
  void set_rs1(u8 rs1);
  std::optional<u8> get_rs1() const;
  MnemonicDescriptor &&with_rs1(u8 rs1) &&;

  // Does this mnemonic have an rs2 position in its instruction format?
  bool allows_rs2() const noexcept;
  // Is the value of rs2 a constant specified by the instruction?
  bool has_rs2() const noexcept;
  void set_rs2(u8 rs2);
  std::optional<u8> get_rs2() const;
  MnemonicDescriptor &&with_rs2(u8 rs2) &&;

  // Does this mnemonic have an rd position in its instruction format?
  bool allows_rd() const noexcept;
  // Is the value of rd a constant specified by the instruction?
  bool has_rd() const noexcept;
  void set_rd(u8 rd);
  std::optional<u8> get_rd() const;
  MnemonicDescriptor &&with_rd(u8 rd) &&;

  // Does this mnemonic have an rd position in its instruction format?
  bool allows_funct3() const noexcept;
  bool allows_funct7() const noexcept;

  // Does this mnemonic have an rd position in its instruction format?
  bool allows_imm() const noexcept;
  // Is the value of rd a constant specified by the instruction?
  bool has_imm() const noexcept;
  void set_imm(u32 imm);
  std::optional<u32> get_imm() const;
  u8 width_imm() const noexcept;
  u8 imm_shift() const noexcept;
  MnemonicDescriptor &&with_imm(u32 imm) &&;

  template <typename Instruction> Instruction encode(Values) const;
  rv_instruction2 encode(Values) const;

protected:
  MnemonicDescriptor(Type type);
  MnemonicDescriptor(Type, uint8_t opcode7);
  struct Flags {
    u8 rs1 : 1 = 0;
    u8 rs2 : 1 = 0;
    u8 rd : 1 = 0;
    // Must enforce mutual exclusion between imm and funct7, because they always occupy the same slot.
    u8 imm : 1 = 0;
  } _flags;
  Type _type = Type::INVALID;
  u8 _opcode7 = 0, _funct3 = 0;
  u8 _rs1 = 0, _rs2 = 0, _rd = 0;
  // immediate requires up to 20 bits, and is multiplexed with funct7.
  uint32_t _imm_or_funct7 = 0;
  std::array<Operand, 3> _operands;
};

struct Mnemonic {
  std::string name;
  MnemonicDescriptor mn;
};

struct MnemonicNameCompare {
  using is_transparent = void;

  bool operator()(const Mnemonic &a, const Mnemonic &b) const { return a.name < b.name; }
  bool operator()(const Mnemonic &a, std::string_view b) const { return a.name < b; }
  bool operator()(std::string_view a, const Mnemonic &b) const { return a < b.name; }
};

using MnemonicSet = fc::vector_set<Mnemonic, MnemonicNameCompare>;
extern const MnemonicSet string_to_mnemonic;

template <> InstructionR MnemonicDescriptor::encode<InstructionR>(Values) const;
template <> InstructionI MnemonicDescriptor::encode<InstructionI>(Values) const;
template <> InstructionS MnemonicDescriptor::encode<InstructionS>(Values) const;
template <> InstructionU MnemonicDescriptor::encode<InstructionU>(Values) const;
template <> InstructionB MnemonicDescriptor::encode<InstructionB>(Values) const;
template <> InstructionJ MnemonicDescriptor::encode<InstructionJ>(Values) const;
} // namespace riscv
