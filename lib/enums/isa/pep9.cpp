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
  case detail::pep9::InstructionType::N_none: [[fallthrough]];
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

QString isa::Pep9::describeMnemonicUsingPlaceholders(Mnemonic mnemonic) {
  switch (mnemonic) {
  case detail::pep9::Mnemonic::STOP: return "Stop execution";
  case detail::pep9::Mnemonic::RET: return "Return from CALL";
  case detail::pep9::Mnemonic::RETTR: return "Return from trap";
  case detail::pep9::Mnemonic::MOVSPA: return "Move SP to A";
  case detail::pep9::Mnemonic::MOVFLGA: return "Move NZVC flags to A<12..15>";
  case detail::pep9::Mnemonic::MOVAFLG: return "Move A<12..15> to NZVC flags";
  case detail::pep9::Mnemonic::NOTA: [[fallthrough]];
  case detail::pep9::Mnemonic::NOTX: return "Bitwise invert r";
  case detail::pep9::Mnemonic::NEGA: [[fallthrough]];
  case detail::pep9::Mnemonic::NEGX: return "Negate r";
  case detail::pep9::Mnemonic::ASLA: [[fallthrough]];
  case detail::pep9::Mnemonic::ASLX: return "Arithmetic shift left r";
  case detail::pep9::Mnemonic::ASRA: [[fallthrough]];
  case detail::pep9::Mnemonic::ASRX: return "Arithmetic shift right r";
  case detail::pep9::Mnemonic::ROLA: [[fallthrough]];
  case detail::pep9::Mnemonic::ROLX: return "Rotate left r";
  case detail::pep9::Mnemonic::RORA: [[fallthrough]];
  case detail::pep9::Mnemonic::RORX: return "Rotate right r";
  case detail::pep9::Mnemonic::BR: return "Branch unconditional";
  case detail::pep9::Mnemonic::BRLE: return "Branch if less than or equal to";
  case detail::pep9::Mnemonic::BRLT: return "Branch if less than";
  case detail::pep9::Mnemonic::BREQ: return "Branch if equal to";
  case detail::pep9::Mnemonic::BRNE: return "Branch if not equal to";
  case detail::pep9::Mnemonic::BRGE: return "Branch if greater than or equal to";
  case detail::pep9::Mnemonic::BRGT: return "Branch if greater than";
  case detail::pep9::Mnemonic::BRV: return "Branch if V";
  case detail::pep9::Mnemonic::BRC: return "Branch if C";
  case detail::pep9::Mnemonic::CALL: return "Call subroutine";
  case detail::pep9::Mnemonic::NOP0: [[fallthrough]];
  case detail::pep9::Mnemonic::NOP1: return "Unary no operation trap";
  case detail::pep9::Mnemonic::NOP: return "Nonunary no operation trap";
  case detail::pep9::Mnemonic::DECI: return "Decimal input trap";
  case detail::pep9::Mnemonic::DECO: return "Decimal output trap";
  case detail::pep9::Mnemonic::HEXO: return "Hexadecimal output trap";
  case detail::pep9::Mnemonic::STRO: return "String output trap";
  case detail::pep9::Mnemonic::ADDSP: return "Add to stack pointer (SP)";
  case detail::pep9::Mnemonic::SUBSP: return "Subtract from stack pointer (SP)";
  case detail::pep9::Mnemonic::ADDA: [[fallthrough]];
  case detail::pep9::Mnemonic::ADDX: return "Add to r";
  case detail::pep9::Mnemonic::SUBA: [[fallthrough]];
  case detail::pep9::Mnemonic::SUBX: return "Subtract from r";
  case detail::pep9::Mnemonic::ANDA: [[fallthrough]];
  case detail::pep9::Mnemonic::ANDX: return "Bitwise AND to r";
  case detail::pep9::Mnemonic::ORA: [[fallthrough]];
  case detail::pep9::Mnemonic::ORX: return "Bitwise OR to r";
  case detail::pep9::Mnemonic::CPWA: [[fallthrough]];
  case detail::pep9::Mnemonic::CPWX: return "Compare word to r";
  case detail::pep9::Mnemonic::CPBA: [[fallthrough]];
  case detail::pep9::Mnemonic::CPBX: return "Compare byte to r<8..15>";
  case detail::pep9::Mnemonic::LDWA: [[fallthrough]];
  case detail::pep9::Mnemonic::LDWX: return "Load word r from memory";
  case detail::pep9::Mnemonic::LDBA: [[fallthrough]];
  case detail::pep9::Mnemonic::LDBX: return "Load byte r<8..15> from memory";
  case detail::pep9::Mnemonic::STWA: [[fallthrough]];
  case detail::pep9::Mnemonic::STWX: return "Store word r to memory";
  case detail::pep9::Mnemonic::STBA: [[fallthrough]];
  case detail::pep9::Mnemonic::STBX: return "Store byte r<8..15> to memory";
  case detail::pep9::Mnemonic::INVALID: return "Illegal instruction";
  }
}

QString isa::Pep9::instructionSpecifierWithPlaceholders(Mnemonic mnemonic) {
  using enum detail::pep9::InstructionType;
  quint8 opcode = static_cast<quint8>(mnemonic);
  QString asBinary = ("00000000" + QString::number(opcode, 2)).right(8);
  switch (opcodeLUT[opcode].instr.type) {
  case R_none: asBinary[7] = 'r'; break;
  case A_ix: asBinary[7] = 'a'; break;
  case AAA_all: [[fallthrough]];
  case AAA_i: asBinary[7] = asBinary[6] = asBinary[5] = 'a'; break;
  case RAAA_all: [[fallthrough]];
  case RAAA_noi:
    asBinary[7] = asBinary[6] = asBinary[5] = 'a';
    asBinary[4] = 'r';
    break;
  default: break;
  }
  return asBinary;
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
