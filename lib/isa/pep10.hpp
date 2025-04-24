/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <QtCore>

namespace isa::detail::pep10 {
Q_NAMESPACE
enum class Mnemonic {
  RET = 0x1,
  SRET = 0x2,
  MOVFLGA = 0x3,
  MOVAFLG = 0x4,
  MOVSPA = 0x5,
  MOVASP = 0x6,
  NOP = 0x7,

  // FAULTS
  UNIMPL,

  NEGA = 0x18,
  NEGX = 0x19,
  ASLA = 0x1A,
  ASLX = 0x1B,
  ASRA = 0x1C,
  ASRX = 0x1D,
  NOTA = 0x1E,
  NOTX = 0x1F,
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
  ADDSP = 0x40,
  SUBSP = 0x48,

  ADDA = 0x50,
  ADDX = 0x58,
  SUBA = 0x60,
  SUBX = 0x68,
  ANDA = 0x70,
  ANDX = 0x78,
  ORA = 0x80,
  ORX = 0x88,
  XORA = 0x90,
  XORX = 0x98,
  CPWA = 0xA0,
  CPWX = 0xA8,
  CPBA = 0xB0,
  CPBX = 0xB8,
  LDWA = 0xC0,
  LDWX = 0xC8,
  LDBA = 0xD0,
  LDBX = 0xD8,
  STWA = 0xE0,
  STWX = 0xE8,
  STBA = 0xF0,
  STBX = 0xF8,
  INVALID = 0x100,
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
  bool valid;
};
constexpr std::array<Opcode, 256> initOpcodes() {
  using M = Mnemonic;
  using T = InstructionType;
  using AM = AddressingMode;
  auto ret = std::array<Opcode, 256>();
  auto add_ix = [&ret](Instruction i) {
    auto base = static_cast<quint8>(i.mnemon);
    ret[base] = {.instr = i, .mode = AM::I, .valid = true};
    ret[base + 1] = {.instr = i, .mode = AM::X, .valid = true};
  };
  auto add_all = [&ret](Instruction i) {
    auto base = static_cast<quint8>(i.mnemon);
    ret[base] = {.instr = i, .mode = AM::I, .valid = i.type != T::RAAA_noi};
    ret[base + 1] = {.instr = i, .mode = AM::D, .valid = true};
    ret[base + 2] = {.instr = i, .mode = AM::N, .valid = true};
    ret[base + 3] = {.instr = i, .mode = AM::S, .valid = true};
    ret[base + 4] = {.instr = i, .mode = AM::SF, .valid = true};
    ret[base + 5] = {.instr = i, .mode = AM::X, .valid = true};
    ret[base + 6] = {.instr = i, .mode = AM::SX, .valid = true};
    ret[base + 7] = {.instr = i, .mode = AM::SFX, .valid = true};
  };

  ret[0x00] = {.instr = {.mnemon = M::INVALID, .type = T::U_none, .unary = true}, .mode = AM::NONE, .valid = false};
  ret[(quint8)M::RET] = {.instr = {.mnemon = M::RET, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::SRET] = {.instr = {.mnemon = M::SRET, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::MOVFLGA] = {
      .instr = {.mnemon = M::MOVFLGA, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::MOVAFLG] = {
      .instr = {.mnemon = M::MOVAFLG, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::MOVSPA] = {
      .instr = {.mnemon = M::MOVSPA, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::MOVASP] = {
      .instr = {.mnemon = M::MOVASP, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::NOP] = {.instr = {.mnemon = M::NOP, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};

  // Gap
  for (int it = (int)M::NOP + 1; it < (int)M::NOTA; it++)
    ret[it] = {.instr = {.mnemon = M::INVALID, .type = T::U_none, .unary = true}, .mode = AM::NONE, .valid = false};

  ret[(quint8)M::NEGA] = {.instr = {.mnemon = M::NEGA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::NEGX] = {.instr = {.mnemon = M::NEGX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::ASLA] = {.instr = {.mnemon = M::ASLA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::ASLX] = {.instr = {.mnemon = M::ASLX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::ASRA] = {.instr = {.mnemon = M::ASRA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::ASRX] = {.instr = {.mnemon = M::ASRX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::NOTA] = {.instr = {.mnemon = M::NOTA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::NOTX] = {.instr = {.mnemon = M::NOTX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::ROLA] = {.instr = {.mnemon = M::ROLA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::ROLX] = {.instr = {.mnemon = M::ROLX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::RORA] = {.instr = {.mnemon = M::RORA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  ret[(quint8)M::RORX] = {.instr = {.mnemon = M::RORX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};

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
  TrapHandler = 0xFFF7,
  Dispatcher = 0xFFF9,
  SystemStackPtr = 0xFFFB,
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
  static constexpr quint8 RegisterCount = 7;
  static constexpr quint8 CSRCount = 4;

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
  static quint8 operandBytes(Mnemonic mnemonic);
  static quint8 operandBytes(quint8 opcode);
  // SCALL and CALL
  static bool isCall(Mnemonic mnemonic);
  static bool isCall(quint8 opcode);
  static bool isTrap(Mnemonic mnemonic);
  static bool isTrap(quint8 opcode);

  static bool isUType(Mnemonic mnemonic);
  static bool isRType(Mnemonic mnemonic);
  static bool isAType(Mnemonic mnemonic);
  static bool isValidATypeAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  static bool isAAAType(Mnemonic mnemonic);
  static bool isValidAAATypeAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  static bool isRAAAType(Mnemonic mnemonic);
  static bool isValidRAAATypeAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  static bool isValidAddressingMode(Mnemonic mnemonic, AddressingMode addr);

  static bool requiresAddressingMode(Mnemonic mnemonic);
  static bool canElideAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  constexpr static std::array<Opcode, 256> opcodeLUT = detail::pep10::initOpcodes();
  static QSet<QString> legalDirectives();
  static bool isLegalDirective(QString directive);
};
} // namespace isa
