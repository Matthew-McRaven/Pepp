#pragma once
#include <QtCore>
namespace isa::detail::pep10 {
Q_NAMESPACE;
enum class Mnemonic {
  RET = 0x0,
  SRET = 0x1,
  MOVSPA = 0x2,
  MOVASP = 0x3,
  MOVFLGA = 0x4,
  MOVAFLG = 0x5,
  NOP = 0x6,

  // FAULTS
  UNIMPL,

  NOTA = 0x18,
  NOTX = 0x19,
  NEGA = 0x1A,
  NEGX = 0x1B,
  ASLA = 0x1C,
  ASLX = 0x1D,
  ASRA = 0x1E,
  ASRX = 0x1F,
  ROLA = 0x20,
  ROLX = 0x21,
  RORA = 0x22,
  RORX = 0x23,
  // STOP,
  BR = 0x24,
  BRLE = 0x26,
  BRLT = 0x28,
  BREQ = 0x2A,
  BRNE = 0x2C,
  BRGE = 0x2E,
  BRGT = 0x30,
  BRV = 0x32,
  BRC = 0x34,
  CALL = 0x36,
  SCALL = 0x38,
  LDWA = 0x40,
  LDWX = 0x48,
  LDBA = 0x50,
  LDBX = 0x58,
  STWA = 0x60,
  STWX = 0x68,
  STBA = 0x70,
  STBX = 0x78,
  CPWA = 0x80,
  CPWX = 0x88,
  CPBA = 0x90,
  CPBX = 0x98,
  ADDA = 0xA0,
  ADDX = 0xA8,
  SUBA = 0xB0,
  SUBX = 0xB8,
  ANDA = 0xC0,
  ANDX = 0xC8,
  ORA = 0xD0,
  ORX = 0xD8,
  XORA = 0xE0,
  XORX = 0xE8,
  ADDSP = 0xF0,
  SUBSP = 0xF8,
  INVALID,
};
Q_ENUM_NS(Mnemonic)

enum class AddressingMode {
  NONE = 0,
  I = 1,
  D = 2,
  N = 4,
  S = 8,
  SF = 16,
  X = 32,
  SX = 64,
  SFX = 128,
  ALL = 255,
  INVALID
};
Q_ENUM_NS(AddressingMode)

enum class InstructionType {
  Invalid,
  U_none,   //?
  R_none,   //?
  A_ix,     //?
  AAA_all,  //?
  AAA_i,    //?
  RAAA_all, //?
  RAAA_noi
};
struct Instruction {
  Mnemonic mnemon;
  InstructionType type;
  bool unary;
};
struct Opcode {
  Instruction instr;
  AddressingMode mode;
};
constexpr std::array<Opcode, 256> initOpcodes() {
  using M = Mnemonic;
  using T = InstructionType;
  using AM = AddressingMode;
  auto ret = std::array<Opcode, 256>();
  auto add_ix = [&ret](Instruction i) {
    auto base = static_cast<quint8>(i.mnemon);
    ret[base] = {.instr = i, .mode = AM::I};
    ret[base + 1] = {.instr = i, .mode = AM::X};
  };
  auto add_all = [&ret](Instruction i) {
    auto base = static_cast<quint8>(i.mnemon);
    ret[base] = {.instr = i, .mode = AM::I};
    ret[base + 1] = {.instr = i, .mode = AM::D};
    ret[base + 2] = {.instr = i, .mode = AM::N};
    ret[base + 3] = {.instr = i, .mode = AM::S};
    ret[base + 4] = {.instr = i, .mode = AM::SF};
    ret[base + 5] = {.instr = i, .mode = AM::X};
    ret[base + 6] = {.instr = i, .mode = AM::SX};
    ret[base + 7] = {.instr = i, .mode = AM::SFX};
  };

  ret[(quint8)M::RET] = {
      .instr = {.mnemon = M::RET, .type = T::U_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::SRET] = {
      .instr = {.mnemon = M::SRET, .type = T::U_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::MOVSPA] = {
      .instr = {.mnemon = M::MOVSPA, .type = T::U_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::MOVASP] = {
      .instr = {.mnemon = M::MOVASP, .type = T::U_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::MOVFLGA] = {
      .instr = {.mnemon = M::MOVFLGA, .type = T::U_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::MOVAFLG] = {
      .instr = {.mnemon = M::MOVAFLG, .type = T::U_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::NOP] = {
      .instr = {.mnemon = M::NOP, .type = T::U_none, .unary = 1},
      .mode = AM::NONE};

  // Gap

  ret[(quint8)M::NOTA] = {
      .instr = {.mnemon = M::NOTA, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::NOTX] = {
      .instr = {.mnemon = M::NOTX, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::NEGA] = {
      .instr = {.mnemon = M::NEGA, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::NEGX] = {
      .instr = {.mnemon = M::NEGX, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::ASLA] = {
      .instr = {.mnemon = M::ASLA, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::ASLX] = {
      .instr = {.mnemon = M::ASLX, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::ASRA] = {
      .instr = {.mnemon = M::ASRA, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::ASRX] = {
      .instr = {.mnemon = M::ASRX, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::ROLA] = {
      .instr = {.mnemon = M::ROLA, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::ROLX] = {
      .instr = {.mnemon = M::ROLX, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::RORA] = {
      .instr = {.mnemon = M::RORA, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};
  ret[(quint8)M::RORX] = {
      .instr = {.mnemon = M::RORX, .type = T::R_none, .unary = 1},
      .mode = AM::NONE};

  add_ix({.mnemon = M::BR, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::BRLE, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::BRLT, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::BREQ, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::BRNE, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::BRGE, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::BRGT, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::BRV, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::BRC, .type = T::A_ix, .unary = 0});
  add_ix({.mnemon = M::CALL, .type = T::A_ix, .unary = 0});
  add_all({.mnemon = M::SCALL, .type = T::AAA_all, .unary = 1});

  add_all({.mnemon = M::LDWA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::LDWX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::LDBA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::LDBX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::STWA, .type = T::RAAA_noi, .unary = 0});
  add_all({.mnemon = M::STWX, .type = T::RAAA_noi, .unary = 0});
  add_all({.mnemon = M::STBA, .type = T::RAAA_noi, .unary = 0});
  add_all({.mnemon = M::STBX, .type = T::RAAA_noi, .unary = 0});
  add_all({.mnemon = M::CPWA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::CPWX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::CPBA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::CPBX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::ADDA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::ADDX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::SUBA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::SUBX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::ANDA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::ANDX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::ORA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::ORX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::XORA, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::XORX, .type = T::RAAA_all, .unary = 0});
  add_all({.mnemon = M::ADDSP, .type = T::AAA_all, .unary = 0});
  add_all({.mnemon = M::SUBSP, .type = T::AAA_all, .unary = 0});
  return ret;
};

enum class Register : quint8 {
  A = 0,
  X = 1,
  SP = 2,
  PC = 3,
  IS = 4,
  OS = 5,
};
Q_ENUM_NS(Register);

enum class CSR : quint8 { N, Z, V, C };
Q_ENUM_NS(CSR);

enum class MemoryVectors : quint16 {
  TrapHandler = 0xFFF6,
  Dispatcher = 0xFFF8,
  SystemStackPtr = 0xFFFA,
  DiskIn = 0xFFFC,
  CharIn = 0xFFFD,
  CharOut = 0xFFFE,
  PwrOff = 0xFFFF,
};
Q_ENUM_NS(MemoryVectors)
} // namespace isa::detail::pep10

namespace isa {
struct Pep10 {
  using Mnemonic = detail::pep10::Mnemonic;
  using AddressingMode = detail::pep10::AddressingMode;
  using InstructionType = detail::pep10::InstructionType;
  using Instruction = detail::pep10::Instruction;
  using Opcode = detail::pep10::Opcode;
  using Register = detail::pep10::Register;
  using CSR = detail::pep10::CSR;
  using MemoryVectors = detail::pep10::MemoryVectors;
  static const quint8 RegisterCount;
  static const quint8 CSRCount;

  static Mnemonic defaultMnemonic();
  static AddressingMode defaultAddressingMode();
  static AddressingMode defaultAddressingMode(Mnemonic mnemonic);
  static quint8 opcode(Mnemonic mnemonic);
  static quint8 opcode(Mnemonic mnemonic, AddressingMode addr);
  static AddressingMode parseAddressingMode(const QString &addr);
  static Mnemonic parseMnemonic(const QString &mnemonic);
  static QString string(Mnemonic mnemonic);
  static QString string(AddressingMode addr);
  // SCALL is a non-unary mnemonic, but a unary opcode;
  static bool isMnemonicUnary(Mnemonic mnemonic);
  static bool isMnemonicUnary(quint8 opcode);
  static bool isOpcodeUnary(Mnemonic mnemonic);
  static bool isOpcodeUnary(quint8 opcode);
  static bool isStore(Mnemonic mnemonic);
  static bool isStore(quint8 opcode);

  static bool isUType(Mnemonic mnemonic);
  static bool isRType(Mnemonic mnemonic);
  static bool isAType(Mnemonic mnemonic);
  static bool isValidATypeAddressingMode(Mnemonic mnemonic,
                                         AddressingMode addr);
  static bool isAAAType(Mnemonic mnemonic);
  static bool isValidAAATypeAddressingMode(Mnemonic mnemonic,
                                           AddressingMode addr);
  static bool isRAAAType(Mnemonic mnemonic);
  static bool isValidRAAATypeAddressingMode(Mnemonic mnemonic,
                                            AddressingMode addr);
  static bool isValidAddressingMode(Mnemonic mnemonic, AddressingMode addr);

  static bool requiresAddressingMode(Mnemonic mnemonic);
  static bool canElideAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  constexpr static std::array<Opcode, 256> opcodeLUT =
      detail::pep10::initOpcodes();
  static bool isLegalDirective(QString directive);
};
} // namespace isa
