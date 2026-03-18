#pragma once
#include <flat/flat_set.hpp>
#include <string>
#include <variant>
#include "core/arch/riscv/isa/rvi.hpp"
namespace riscv {

class MnemonicU {
public:
  MnemonicU(uint8_t opcode7) noexcept;

  bool has_immediate() const noexcept;
  // Only keeps high-order 20 bits of imm.
  void set_immediate(uint32_t imm);
  std::optional<uint32_t> get_immediate() const;
  MnemonicU &&with_immediate(uint32_t imm) &&;

  bool has_rd() const noexcept;
  void set_rd(uint8_t rd);
  std::optional<uint8_t> get_rd() const;
  MnemonicU &&with_rd(uint8_t rd) &&;

  InstructionU to_instruction() const noexcept;

protected:
  struct Flags {
    uint8_t imm : 1 = 0;
    uint8_t rd : 1 = 0;
  } _flags{};
  static_assert(sizeof(Flags) == 1, "Flags should be 1 byte");
  uint8_t _opcode7 = 0, _rd5 = 0;
  uint32_t _imm20 = 0;
};

class MnemonicJ {
public:
  MnemonicJ(uint8_t opcode7) noexcept;

  bool has_immediate() const noexcept;
  void set_immediate(uint32_t imm);
  std::optional<uint32_t> get_immediate() const;
  MnemonicJ &&with_immediate(uint32_t imm) &&;

  bool has_rd() const noexcept;
  void set_rd(uint8_t rd);
  std::optional<uint8_t> get_rd() const;
  MnemonicJ &&with_rd(uint8_t rd) &&;

  InstructionJ to_instruction() const noexcept;

protected:
  struct Flags {
    uint8_t imm : 1 = 0;
    uint8_t rd : 1 = 0;
  } _flags{};
  static_assert(sizeof(Flags) == 1, "Flags should be 1 byte");
  uint8_t _opcode7 = 0, _rd5 = 0;
  uint32_t _imm20 = 0;
};

class MnemonicB {
public:
  MnemonicB(uint8_t opcode7, uint8_t funct3) noexcept;

  // 12-bit immediate shifted left by 1, so [12:1]
  bool has_immediate() const noexcept;
  void set_immediate(uint16_t imm);
  std::optional<uint16_t> get_immediate() const;
  MnemonicB &&with_immediate(uint16_t imm) &&;

  bool has_rs1() const noexcept;
  void set_rs1(uint8_t rs1);
  std::optional<uint8_t> get_rs1() const;
  MnemonicB &&with_rs1(uint8_t rs1) &&;

  bool has_rs2() const noexcept;
  void set_rs2(uint8_t rs2);
  std::optional<uint8_t> get_rs2() const;
  MnemonicB &&with_rs2(uint8_t rs2) &&;

  InstructionB to_instruction() const noexcept;

protected:
  struct Flags {
    uint8_t imm : 1 = 0;
    uint8_t rs1 : 1 = 0;
    uint8_t rs2 : 1 = 0;
  } _flags{};
  static_assert(sizeof(Flags) == 1, "Flags should be 1 byte");
  uint8_t _opcode7 = 0, _funct3 = 0, _rs15 = 0, _rs25 = 0;
  uint16_t _imm12 = 0;
};

class MnemonicI {
public:
  MnemonicI(uint8_t opcode7, uint8_t funct3) noexcept;

  bool has_immediate() const noexcept;
  void set_immediate(uint16_t imm);
  std::optional<uint16_t> get_immediate() const;
  MnemonicI &&with_immediate(uint16_t imm) &&;

  bool has_rs1() const noexcept;
  void set_rs1(uint8_t rs1);
  std::optional<uint8_t> get_rs1() const;
  MnemonicI &&with_rs1(uint8_t rs1) &&;

  bool has_rd() const noexcept;
  void set_rd(uint8_t rd);
  std::optional<uint8_t> get_rd() const;
  MnemonicI &&with_rd(uint8_t rd) &&;

  InstructionI to_instruction() const noexcept;

protected:
  struct Flags {
    uint8_t imm : 1 = 0;
    uint8_t rs1 : 1 = 0;
    uint8_t rd : 1 = 0;
  } _flags{};
  static_assert(sizeof(Flags) == 1, "Flags should be 1 byte");
  uint8_t _opcode7 = 0, _funct3 = 0, _rs15 = 0, _rd5 = 0;
  uint16_t _imm12 = 0;
};

class ConstantShiftMnemonic : public MnemonicI {
public:
  ConstantShiftMnemonic(uint8_t opcode7, uint8_t funct3) noexcept;

  void set_shift_type(uint8_t shift_type);
  std::optional<uint8_t> get_shift_type() const;
  ConstantShiftMnemonic &&with_shift_type(uint8_t shift_type) &&;

  void set_shamt(uint8_t shamt);
  std::optional<uint8_t> get_shamt() const;
  ConstantShiftMnemonic &&with_shamt(uint8_t shamt) &&;
};

class FenceFormat : public MnemonicI {
public:
  FenceFormat() noexcept;

  void set_fm(uint8_t fm);
  std::optional<uint8_t> get_fm() const;
  FenceFormat &&with_fm(uint8_t fm) &&;

  void set_pred(uint8_t pred);
  std::optional<uint8_t> get_pred() const;
  FenceFormat &&with_pred(uint8_t pred) &&;

  void set_succ(uint8_t succ);
  std::optional<uint8_t> get_succ() const;
  FenceFormat &&with_succ(uint8_t succ) &&;

  static const uint8_t merge_iorw(bool i, bool o, bool r, bool w) noexcept;
};

class MnemonicR {
public:
  MnemonicR(uint8_t opcode7, uint8_t funct3, uint8_t funct7) noexcept;

  bool has_rs1() const noexcept;
  void set_rs1(uint8_t rs1);
  std::optional<uint8_t> get_rs1() const;
  MnemonicR &&with_rs1(uint8_t rs1) &&;

  bool has_rs2() const noexcept;
  void set_rs2(uint8_t rs2);
  std::optional<uint8_t> get_rs2() const;
  MnemonicR &&with_rs2(uint8_t rs2) &&;

  bool has_rd() const noexcept;
  void set_rd(uint8_t rd);
  std::optional<uint8_t> get_rd() const;
  MnemonicR &&with_rd(uint8_t rd) &&;

  InstructionR to_instruction() const noexcept;

private:
  struct Flags {
    uint8_t rs1 : 1 = 0;
    uint8_t rs2 : 1 = 0;
    uint8_t rd : 1 = 0;
  } _flags{};
  static_assert(sizeof(Flags) == 1, "Flags should be 1 byte");
  uint8_t _opcode7 = 0, _funct3 = 0, _funct7 = 0, _rs15 = 0, _rs25 = 0, _rd5 = 0;
};

class MnemonicS {
public:
  MnemonicS(uint8_t opcode7, uint8_t funct3) noexcept;

  bool has_immediate() const noexcept;
  void set_immediate(uint16_t imm);
  std::optional<uint16_t> get_immediate() const;
  MnemonicS &&with_immediate(uint16_t imm) &&;

  bool has_rs1() const noexcept;
  void set_rs1(uint8_t rs1);
  std::optional<uint8_t> get_rs1() const;
  MnemonicS &&with_rs1(uint8_t rs1) &&;

  bool has_rs2() const noexcept;
  void set_rs2(uint8_t rs2);
  std::optional<uint8_t> get_rs2() const;
  MnemonicS &&with_rs2(uint8_t rs2) &&;

  InstructionS to_instruction() const noexcept;

private:
  struct Flags {
    uint8_t imm : 1 = 0;
    uint8_t rs1 : 1 = 0;
    uint8_t rs2 : 1 = 0;
  } _flags{};
  static_assert(sizeof(Flags) == 1, "Flags should be 1 byte");
  uint8_t _opcode7 = 0, _funct3 = 0, _rs15 = 0, _rs25 = 0;
  uint16_t _imm12 = 0;
};

struct Mnemonic {
  std::string name;
  std::variant<std::monostate, MnemonicU, MnemonicJ, MnemonicB, MnemonicR, MnemonicS, MnemonicI> variant;
  bool operator==(const Mnemonic &other) const noexcept = default;
};

struct MnemonicNameCompare {
  using is_transparent = void;

  bool operator()(const Mnemonic &a, const Mnemonic &b) const { return a.name < b.name; }
  bool operator()(const Mnemonic &a, std::string_view b) const { return a.name < b; }
  bool operator()(std::string_view a, const Mnemonic &b) const { return a < b.name; }
};

using MnemonicSet = fc::vector_set<Mnemonic, MnemonicNameCompare>;
extern const MnemonicSet string_to_mnemonic;
// extern const std::map<Mnemonic, std::string> mnemonic_to_string;
} // namespace riscv
