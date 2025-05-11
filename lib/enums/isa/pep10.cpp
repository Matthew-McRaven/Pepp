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

#include "pep10.hpp"
#include <QMetaEnum>
#include "pep_shared.hpp"

isa::Pep10::Mnemonic isa::Pep10::defaultMnemonic() { return Mnemonic::INVALID; }

isa::Pep10::AddressingMode isa::Pep10::defaultAddressingMode() { return AddressingMode::INVALID; }

isa::Pep10::AddressingMode isa::Pep10::defaultAddressingMode(Mnemonic mnemonic) {
  if (isAType(mnemonic)) return AddressingMode::I;
  else return defaultAddressingMode();
}

quint8 isa::Pep10::opcode(Mnemonic mnemonic) { return isa::detail::opcode(mnemonic); }

quint8 isa::Pep10::opcode(Mnemonic mnemonic, AddressingMode addr) { return isa::detail::opcode(mnemonic, addr); }

isa::Pep10::AddressingMode isa::Pep10::parseAddressingMode(const QString &addr) {
  return isa::detail::parseAddressingMode<AddressingMode>(addr);
}

isa::Pep10::Mnemonic isa::Pep10::parseMnemonic(const QString &mnemonic) {
  return isa::detail::parseMnemonic<Mnemonic>(mnemonic);
}

QString isa::Pep10::string(Mnemonic mnemonic) { return isa::detail::stringMnemonic(mnemonic); }

QString isa::Pep10::string(AddressingMode addr) { return isa::detail::stringAddr(addr); }

bool isa::Pep10::isMnemonicUnary(Mnemonic mnemonic) { return isMnemonicUnary(opcode(mnemonic)); }

bool isa::Pep10::isMnemonicUnary(quint8 opcode) {
  using T = detail::pep10::InstructionType;
  auto type = opcodeLUT[opcode].instr.type;
  return type == T::R_none || type == T::U_none;
}

bool isa::Pep10::isOpcodeUnary(Mnemonic mnemonic) { return isOpcodeUnary(opcode(mnemonic)); }

bool isa::Pep10::isOpcodeUnary(quint8 opcode) { return opcodeLUT[opcode].instr.unary; }

bool isa::Pep10::isStore(Mnemonic mnemonic) { return isa::detail::isStore(mnemonic); }

bool isa::Pep10::isStore(quint8 opcode) { return isStore(opcodeLUT[opcode].instr.mnemon); }

quint8 isa::Pep10::operandBytes(Mnemonic mnemonic) {
  if (isMnemonicUnary(mnemonic)) return 0;
  switch (mnemonic) {
  case Mnemonic::LDBA: [[fallthrough]];
  case Mnemonic::LDBX: [[fallthrough]];
  case Mnemonic::CPBA: [[fallthrough]];
  case Mnemonic::CPBX: return 1;
  default: return 2;
  }
}

quint8 isa::Pep10::operandBytes(quint8 opcode) { return operandBytes(opcodeLUT[opcode].instr.mnemon); }

bool isa::Pep10::isCall(Mnemonic mnemonic) { return mnemonic == Mnemonic::CALL || mnemonic == Mnemonic::SCALL; }

bool isa::Pep10::isCall(quint8 opcode) { return isCall(opcodeLUT[opcode].instr.mnemon); }

bool isa::Pep10::isTrap(Mnemonic mnemonic) { return mnemonic == Mnemonic::SCALL; }

bool isa::Pep10::isTrap(quint8 opcode) { return isTrap(opcodeLUT[opcode].instr.mnemon); }

bool isa::Pep10::isUType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::U_none;
}

bool isa::Pep10::isRType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::R_none;
}

bool isa::Pep10::isAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::A_ix;
}

bool isa::Pep10::isValidATypeAddressingMode(Mnemonic, AddressingMode addr) {
  using AM = AddressingMode;
  return addr == AM::I || addr == AM::X;
}

bool isa::Pep10::isAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::AAA_all || type == T::AAA_i;
}

bool isa::Pep10::isValidAAATypeAddressingMode(Mnemonic, AddressingMode addr) {
  using AM = AddressingMode;
  return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE);
}

bool isa::Pep10::isRAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::RAAA_all || type == T::RAAA_noi;
}

bool isa::Pep10::isValidRAAATypeAddressingMode(Mnemonic mnemonic, AddressingMode addr) {
  using T = InstructionType;
  using AM = AddressingMode;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE || (type == T::RAAA_noi && addr == AM::I));
}

bool isa::Pep10::isValidAddressingMode(Mnemonic mnemonic, AddressingMode addr) {
  using T = InstructionType;
  using AM = AddressingMode;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  switch (type) {
  case detail::pep10::InstructionType::Invalid: [[fallthrough]];
  case detail::pep10::InstructionType::U_none: [[fallthrough]];
  case detail::pep10::InstructionType::R_none: return false;
  case detail::pep10::InstructionType::A_ix: return addr == AM::X || addr == AM::I;
  case detail::pep10::InstructionType::AAA_i: return addr == AM::I;
  case detail::pep10::InstructionType::AAA_all: [[fallthrough]];
  case detail::pep10::InstructionType::RAAA_all: return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE);
  case detail::pep10::InstructionType::RAAA_noi:
    return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE || addr == AM::I);
  }
  return false;
}

bool isa::Pep10::decodeOperandAsSigned(quint8 opcode) {
  using enum detail::pep10::AddressingMode;
  switch (opcodeLUT[opcode].mode) {
  case I: [[fallthrough]];
  case S: [[fallthrough]];
  case SF: [[fallthrough]];
  case SX: [[fallthrough]];
  case SFX: return true;
  default: return false;
  }
}

bool isa::Pep10::requiresAddressingMode(Mnemonic mnemonic) { return isAAAType(mnemonic) | isRAAAType(mnemonic); }

bool isa::Pep10::canElideAddressingMode(Mnemonic mnemonic, AddressingMode addr) {
  return isAType(mnemonic) && addr == AddressingMode::I;
}

QSet<QString> isa::Pep10::legalDirectives() {
  static const auto valid = QSet<QString>{"ALIGN", "ASCII",  "BLOCK", "BYTE",  "EQUATE",  "EXPORT", "IMPORT",
                                          "INPUT", "OUTPUT", "ORG",   "SCALL", "SECTION", "USCALL", "WORD"};
  return valid;
}

bool isa::Pep10::isLegalDirective(QString directive) { return legalDirectives().contains(directive.toUpper()); }
