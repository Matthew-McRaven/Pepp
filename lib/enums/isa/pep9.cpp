/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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

#include "pep9.hpp"
#include <QMetaEnum>
#include "pep_shared.hpp"

isa::Pep9::Mnemonic isa::Pep9::defaultMnemonic() { return Mnemonic::INVALID; }

isa::Pep9::AddressingMode isa::Pep9::defaultAddressingMode() { return AddressingMode::INVALID; }

isa::Pep9::AddressingMode isa::Pep9::defaultAddressingMode(Mnemonic mnemonic) {
  if (isAType(mnemonic)) return AddressingMode::I;
  else return defaultAddressingMode();
}

quint8 isa::Pep9::opcode(Mnemonic mnemonic) { return isa::detail::opcode(mnemonic); }

quint8 isa::Pep9::opcode(Mnemonic mnemonic, AddressingMode addr) { return isa::detail::opcode(mnemonic, addr); }

isa::Pep9::AddressingMode isa::Pep9::parseAddressingMode(const QString &addr) {
  return isa::detail::parseAddressingMode<AddressingMode>(addr);
}

isa::Pep9::Mnemonic isa::Pep9::parseMnemonic(const QString &mnemonic) {
  return isa::detail::parseMnemonic<Mnemonic>(mnemonic);
}

QString isa::Pep9::string(Mnemonic mnemonic) { return isa::detail::stringMnemonic(mnemonic); }

QString isa::Pep9::string(AddressingMode addr) { return isa::detail::stringAddr(addr); }

bool isa::Pep9::isMnemonicUnary(Mnemonic mnemonic) { return isMnemonicUnary(opcode(mnemonic)); }

bool isa::Pep9::isMnemonicUnary(quint8 opcode) {
  using T = detail::pep9::InstructionType;
  auto type = opcodeLUT[opcode].instr.type;
  return type == T::R_none || type == T::U_none;
}

bool isa::Pep9::isOpcodeUnary(Mnemonic mnemonic) { return isOpcodeUnary(opcode(mnemonic)); }

bool isa::Pep9::isOpcodeUnary(quint8 opcode) { return opcodeLUT[opcode].instr.unary; }

bool isa::Pep9::isStore(Mnemonic mnemonic) { return isa::detail::isStore(mnemonic); }

bool isa::Pep9::isStore(quint8 opcode) { return isStore(opcodeLUT[opcode].instr.mnemon); }

quint8 isa::Pep9::operandBytes(Mnemonic mnemonic) {
  if (isMnemonicUnary(mnemonic)) return 0;
  switch (mnemonic) {
  case Mnemonic::LDBA: [[fallthrough]];
  case Mnemonic::LDBX: [[fallthrough]];
  case Mnemonic::CPBA: [[fallthrough]];
  case Mnemonic::CPBX: return 1;
  default: return 2;
  }
}

bool isa::Pep9::isCall(quint8 opcode) { return isCall(opcodeLUT[opcode].instr.mnemon); }

bool isa::Pep9::isCall(Mnemonic mnemonic) {
  switch (mnemonic) {
  case Mnemonic::NOP0: [[fallthrough]];
  case Mnemonic::NOP1: [[fallthrough]];
  case Mnemonic::CALL: [[fallthrough]];
  case Mnemonic::NOP: [[fallthrough]];
  case Mnemonic::DECI: [[fallthrough]];
  case Mnemonic::DECO: [[fallthrough]];
  case Mnemonic::HEXO: [[fallthrough]];
  case Mnemonic::STRO: return true;
  default: return false;
  }
}

bool isa::Pep9::isTrap(quint8 opcode) { return isTrap(opcodeLUT[opcode].instr.mnemon); }

bool isa::Pep9::isTrap(Mnemonic mnemonic) {
  switch (mnemonic) {
  case Mnemonic::NOP0: [[fallthrough]];
  case Mnemonic::NOP1: [[fallthrough]];
  case Mnemonic::NOP: [[fallthrough]];
  case Mnemonic::DECI: [[fallthrough]];
  case Mnemonic::DECO: [[fallthrough]];
  case Mnemonic::HEXO: [[fallthrough]];
  case Mnemonic::STRO: return true;
  default: return false;
  }
}

quint8 isa::Pep9::operandBytes(quint8 opcode) { return operandBytes(opcodeLUT[opcode].instr.mnemon); }

bool isa::Pep9::isUType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::U_none;
}

bool isa::Pep9::isRType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::R_none;
}

bool isa::Pep9::isAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::A_ix;
}

bool isa::Pep9::isValidATypeAddressingMode(Mnemonic, AddressingMode addr) {
  using AM = AddressingMode;
  return addr == AM::I || addr == AM::X;
}

bool isa::Pep9::isAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::AAA_all || type == T::AAA_i || type == T::AAA_stro || type == T::AAA_noi;
}

bool isa::Pep9::isValidAAATypeAddressingMode(Mnemonic, AddressingMode addr) {
  using AM = AddressingMode;
  return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE);
}

bool isa::Pep9::isRAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::RAAA_all || type == T::RAAA_noi;
}

bool isa::Pep9::isValidRAAATypeAddressingMode(Mnemonic mnemonic, AddressingMode addr) {
  using T = InstructionType;
  using AM = AddressingMode;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE || (type == T::RAAA_noi && addr == AM::I));
}

bool isa::Pep9::isValidAddressingMode(Mnemonic mnemonic, AddressingMode addr) {
  using T = InstructionType;
  using AM = AddressingMode;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  switch (type) {
  case detail::pep9::InstructionType::Invalid: [[fallthrough]];
  case detail::pep9::InstructionType::U_none: [[fallthrough]];
  case detail::pep9::InstructionType::R_none: return false;
  case detail::pep9::InstructionType::A_ix: return addr == AM::X || addr == AM::I;

  case detail::pep9::InstructionType::AAA_i: return addr == AM::I;
  case detail::pep9::InstructionType::AAA_stro:
    return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE || addr == AM::I || addr == AM::SX ||
             addr == AM::SFX);
  case detail::pep9::InstructionType::AAA_all: [[fallthrough]];
  case detail::pep9::InstructionType::RAAA_all: return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE);
  case detail::pep9::InstructionType::AAA_noi: [[fallthrough]];
  case detail::pep9::InstructionType::RAAA_noi:
    return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE || addr == AM::I);
  }
  return false;
}

bool isa::Pep9::decodeOperandAsSigned(quint8 opcode) {
  using enum detail::pep9::AddressingMode;
  switch (opcodeLUT[opcode].mode) {
  case I: [[fallthrough]];
  case S: [[fallthrough]];
  case SF: [[fallthrough]];
  case SX: [[fallthrough]];
  case SFX: return true;
  default: return false;
  }
}

bool isa::Pep9::requiresAddressingMode(Mnemonic mnemonic) { return isAAAType(mnemonic) | isRAAAType(mnemonic); }

bool isa::Pep9::canElideAddressingMode(Mnemonic mnemonic, AddressingMode addr) {
  return isAType(mnemonic) && addr == AddressingMode::I;
}

QSet<QString> isa::Pep9::legalDirectives() {
  static const auto valid = QSet<QString>{"ADDRSS", "ALIGN", "ASCII", "BLOCK", "BURN", "BYTE", "END", "EQUATE", "WORD"};
  return valid;
}

bool isa::Pep9::isLegalDirective(QString directive) { return legalDirectives().contains(directive.toUpper()); }
