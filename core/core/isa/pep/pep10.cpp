/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/isa/pep/pep10.hpp"
#include <bitset>
#include "core/libs/bitmanip/strings.hpp"
#include "core/isa/pep/pep_shared.hpp"

static auto register_maps() {
  std::map<isa::Pep10::Register, std::string> reg_to_str;
  std::unordered_map<std::string, isa::Pep10::Register, pepp::bts::ci_hash, pepp::bts::ci_eq> str_to_reg;
  auto insert = [](auto &reg_to_str, auto &str_to_reg, isa::Pep10::Register reg, const char *str) {
    reg_to_str[reg] = str;
    str_to_reg[str] = reg;
  };

  using enum isa::Pep10::Register;
  insert(reg_to_str, str_to_reg, A, "A");
  insert(reg_to_str, str_to_reg, X, "X");
  insert(reg_to_str, str_to_reg, SP, "SP");
  insert(reg_to_str, str_to_reg, PC, "PC");
  insert(reg_to_str, str_to_reg, IS, "IS");
  insert(reg_to_str, str_to_reg, OS, "OS");
  insert(reg_to_str, str_to_reg, INVALID, "invalid");
  return std::make_pair(reg_to_str, str_to_reg);
}

static auto am_maps() {
  std::map<isa::Pep10::AddressingMode, std::string> am_to_str;
  std::unordered_map<std::string, isa::Pep10::AddressingMode, pepp::bts::ci_hash, pepp::bts::ci_eq> str_to_am;

  auto insert = [](auto &am_to_str, auto &str_to_am, isa::Pep10::AddressingMode am, const char *str) {
    am_to_str[am] = str;
    str_to_am[str] = am;
  };

  using enum isa::Pep10::AddressingMode;
  insert(am_to_str, str_to_am, NONE, "none");
  insert(am_to_str, str_to_am, I, "I");
  insert(am_to_str, str_to_am, D, "D");
  insert(am_to_str, str_to_am, N, "N");
  insert(am_to_str, str_to_am, S, "S");
  insert(am_to_str, str_to_am, SF, "SF");
  insert(am_to_str, str_to_am, X, "X");
  insert(am_to_str, str_to_am, SX, "SX");
  insert(am_to_str, str_to_am, SFX, "SFX");
  insert(am_to_str, str_to_am, INVALID, "invalid");
  return std::make_pair(am_to_str, str_to_am);
}

static auto mnemonic_maps() {
  std::map<isa::Pep10::Mnemonic, std::string> mn_to_str;
  std::unordered_map<std::string, isa::Pep10::Mnemonic, pepp::bts::ci_hash, pepp::bts::ci_eq> str_to_mn;

  auto insert = [](auto &mn_to_str, auto &str_to_mn, isa::Pep10::Mnemonic mn, const char *str) {
    mn_to_str[mn] = str;
    str_to_mn[str] = mn;
  };
  using enum isa::Pep10::Mnemonic;
  insert(mn_to_str, str_to_mn, RET, "RET");
  insert(mn_to_str, str_to_mn, SRET, "SRET");
  insert(mn_to_str, str_to_mn, MOVFLGA, "MOVFLGA");
  insert(mn_to_str, str_to_mn, MOVAFLG, "MOVAFLG");
  insert(mn_to_str, str_to_mn, MOVSPA, "MOVSPA");
  insert(mn_to_str, str_to_mn, MOVASP, "MOVASP");
  insert(mn_to_str, str_to_mn, NOP, "NOP");
  insert(mn_to_str, str_to_mn, NEGA, "NEGA");
  insert(mn_to_str, str_to_mn, NEGX, "NEGX");
  insert(mn_to_str, str_to_mn, ASLA, "ASLA");
  insert(mn_to_str, str_to_mn, ASLX, "ASLX");
  insert(mn_to_str, str_to_mn, ASRA, "ASRA");
  insert(mn_to_str, str_to_mn, ASRX, "ASRX");
  insert(mn_to_str, str_to_mn, NOTA, "NOTA");
  insert(mn_to_str, str_to_mn, NOTX, "NOTX");
  insert(mn_to_str, str_to_mn, ROLA, "ROLA");
  insert(mn_to_str, str_to_mn, ROLX, "ROLX");
  insert(mn_to_str, str_to_mn, RORA, "RORA");
  insert(mn_to_str, str_to_mn, RORX, "RORX");
  insert(mn_to_str, str_to_mn, BR, "BR");
  insert(mn_to_str, str_to_mn, BRLE, "BRLE");
  insert(mn_to_str, str_to_mn, BRLT, "BRLT");
  insert(mn_to_str, str_to_mn, BREQ, "BREQ");
  insert(mn_to_str, str_to_mn, BRNE, "BRNE");
  insert(mn_to_str, str_to_mn, BRGE, "BRGE");
  insert(mn_to_str, str_to_mn, BRGT, "BRGT");
  insert(mn_to_str, str_to_mn, BRV, "BRV");
  insert(mn_to_str, str_to_mn, BRC, "BRC");
  insert(mn_to_str, str_to_mn, CALL, "CALL");
  insert(mn_to_str, str_to_mn, SCALL, "SCALL");
  insert(mn_to_str, str_to_mn, ADDSP, "ADDSP");
  insert(mn_to_str, str_to_mn, SUBSP, "SUBSP");
  insert(mn_to_str, str_to_mn, ADDA, "ADDA");
  insert(mn_to_str, str_to_mn, ADDX, "ADDX");
  insert(mn_to_str, str_to_mn, SUBA, "SUBA");
  insert(mn_to_str, str_to_mn, SUBX, "SUBX");
  insert(mn_to_str, str_to_mn, ANDA, "ANDA");
  insert(mn_to_str, str_to_mn, ANDX, "ANDX");
  insert(mn_to_str, str_to_mn, ORA, "ORA");
  insert(mn_to_str, str_to_mn, ORX, "ORX");
  insert(mn_to_str, str_to_mn, XORA, "XORA");
  insert(mn_to_str, str_to_mn, XORX, "XORX");
  insert(mn_to_str, str_to_mn, CPWA, "CPWA");
  insert(mn_to_str, str_to_mn, CPWX, "CPWX");
  insert(mn_to_str, str_to_mn, CPBA, "CPBA");
  insert(mn_to_str, str_to_mn, CPBX, "CPBX");
  insert(mn_to_str, str_to_mn, LDWA, "LDWA");
  insert(mn_to_str, str_to_mn, LDWX, "LDWX");
  insert(mn_to_str, str_to_mn, LDBA, "LDBA");
  insert(mn_to_str, str_to_mn, LDBX, "LDBX");
  insert(mn_to_str, str_to_mn, STWA, "STWA");
  insert(mn_to_str, str_to_mn, STWX, "STWX");
  insert(mn_to_str, str_to_mn, STBA, "STBA");
  insert(mn_to_str, str_to_mn, STBX, "STBX");
  return std::make_pair(mn_to_str, str_to_mn);
}
isa::Pep10::Mnemonic isa::Pep10::defaultMnemonic() { return Mnemonic::INVALID; }

isa::Pep10::AddressingMode isa::Pep10::defaultAddressingMode() { return AddressingMode::INVALID; }

isa::Pep10::AddressingMode isa::Pep10::defaultAddressingMode(Mnemonic mnemonic) {
  if (isAType(mnemonic)) return AddressingMode::I;
  else return defaultAddressingMode();
}

std::vector<std::string> const &isa::Pep10::mnemonics() {
  static const std::vector<std::string> ret = []() {
    std::vector<std::string> lst;
    auto mn_to_str = mnemonic_maps().first;
    lst.reserve(mn_to_str.size());
    for (const auto &[mn, str] : mn_to_str) lst.emplace_back(str);
    return lst;
  }();
  return ret;
}

uint8_t isa::Pep10::opcode(Mnemonic mnemonic) { return isa::detail::opcode(mnemonic); }

uint8_t isa::Pep10::opcode(Mnemonic mnemonic, AddressingMode addr) { return isa::detail::opcode(mnemonic, addr); }

isa::Pep10::AddressingMode isa::Pep10::parseAddressingMode(const std::string &addr) {
  auto str_to_am = isa::Pep10::string_to_addressmode();
  auto it = str_to_am.find(addr);
  if (it != str_to_am.end()) return it->second;
  else return isa::Pep10::AddressingMode::INVALID;
}

isa::Pep10::Mnemonic isa::Pep10::parseMnemonic(const std::string &mnemonic) {
  auto str_to_mn = isa::Pep10::string_to_mnemonic();
  auto it = str_to_mn.find(mnemonic);
  if (it != str_to_mn.end()) return it->second;
  else return isa::Pep10::Mnemonic::INVALID;
}

isa::Pep10::Register isa::Pep10::parseRegister(const std::string &reg) {
  auto str_to_reg = isa::Pep10::string_to_register();
  auto it = str_to_reg.find(reg);
  if (it != str_to_reg.end()) return it->second;
  else return isa::Pep10::Register::INVALID;
}

std::string isa::Pep10::string(Mnemonic mnemonic) {
  auto mn_to_str = isa::Pep10::mnemonic_to_string();
  return mn_to_str.at(mnemonic);
}

std::string isa::Pep10::string(AddressingMode addr) {
  auto am_to_str = isa::Pep10::addressmode_to_string();
  auto it = am_to_str.find(addr);
  if (it == am_to_str.end()) return "invalid";
  std::string ret = it->second;
  std::transform(ret.begin(), ret.end(), ret.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ret;
}

std::string isa::Pep10::string(Register reg) {
  auto reg_to_str = isa::Pep10::register_to_string();
  return reg_to_str.at(reg);
}

bool isa::Pep10::isMnemonicUnary(Mnemonic mnemonic) { return isMnemonicUnary(opcode(mnemonic)); }

bool isa::Pep10::isMnemonicUnary(uint8_t opcode) {
  using T = detail::pep10::InstructionType;
  auto type = opcodeLUT[opcode].instr.type;
  return type == T::R_none || type == T::U_none;
}

bool isa::Pep10::isOpcodeUnary(Mnemonic mnemonic) { return isOpcodeUnary(opcode(mnemonic)); }

bool isa::Pep10::isOpcodeUnary(uint8_t opcode) { return opcodeLUT[opcode].instr.unary; }

bool isa::Pep10::isStore(Mnemonic mnemonic) { return isa::detail::isStore(mnemonic); }

bool isa::Pep10::isStore(uint8_t opcode) { return isStore(opcodeLUT[opcode].instr.mnemon); }

uint8_t isa::Pep10::operandBytes(Mnemonic mnemonic) {
  if (isMnemonicUnary(mnemonic)) return 0;
  switch (mnemonic) {
  case Mnemonic::LDBA: [[fallthrough]];
  case Mnemonic::LDBX: [[fallthrough]];
  case Mnemonic::CPBA: [[fallthrough]];
  case Mnemonic::CPBX: return 1;
  default: return 2;
  }
}

uint8_t isa::Pep10::operandBytes(uint8_t opcode) { return operandBytes(opcodeLUT[opcode].instr.mnemon); }

bool isa::Pep10::isCall(Mnemonic mnemonic) { return mnemonic == Mnemonic::CALL || mnemonic == Mnemonic::SCALL; }

bool isa::Pep10::isCall(uint8_t opcode) { return isCall(opcodeLUT[opcode].instr.mnemon); }

bool isa::Pep10::isTrap(Mnemonic mnemonic) { return mnemonic == Mnemonic::SCALL; }

bool isa::Pep10::isTrap(uint8_t opcode) { return isTrap(opcodeLUT[opcode].instr.mnemon); }

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

bool isa::Pep10::decodeOperandAsSigned(uint8_t opcode) {
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

std::string isa::Pep10::describeMnemonicUsingPlaceholders(Mnemonic mnemonic) {
  switch (mnemonic) {
  case detail::pep10::Mnemonic::RET: return "Return from CALL";
  case detail::pep10::Mnemonic::SRET: return "Return from system CALL";
  case detail::pep10::Mnemonic::MOVFLGA: return "Move NZVC flags to A[12:15]";
  case detail::pep10::Mnemonic::MOVAFLG: return "Move A[12:15] to NZVC flags";
  case detail::pep10::Mnemonic::MOVSPA: return "Move SP to A";
  case detail::pep10::Mnemonic::MOVASP: return "Move A to SP";
  case detail::pep10::Mnemonic::NOP: return "No operation";
  case detail::pep10::Mnemonic::NEGA: [[fallthrough]];
  case detail::pep10::Mnemonic::NEGX: return "Negate r";
  case detail::pep10::Mnemonic::ASLA: [[fallthrough]];
  case detail::pep10::Mnemonic::ASLX: return "Arithmetic shift left r";
  case detail::pep10::Mnemonic::ASRA: [[fallthrough]];
  case detail::pep10::Mnemonic::ASRX: return "Arithmetic shift right r";
  case detail::pep10::Mnemonic::NOTA: [[fallthrough]];
  case detail::pep10::Mnemonic::NOTX: return "Bitwise Not r";
  case detail::pep10::Mnemonic::ROLA: [[fallthrough]];
  case detail::pep10::Mnemonic::ROLX: return "Rotate left r";
  case detail::pep10::Mnemonic::RORA: [[fallthrough]];
  case detail::pep10::Mnemonic::RORX: return "Rotate right r";
  case detail::pep10::Mnemonic::BR: return "Branch unconditional";
  case detail::pep10::Mnemonic::BRLE: return "Branch if less than or equal to";
  case detail::pep10::Mnemonic::BRLT: return "Branch if less than";
  case detail::pep10::Mnemonic::BREQ: return "Branch if equal to";
  case detail::pep10::Mnemonic::BRNE: return "Branch if not equal to";
  case detail::pep10::Mnemonic::BRGE: return "Branch if greater than or equal to";
  case detail::pep10::Mnemonic::BRGT: return "Branch if greater than";
  case detail::pep10::Mnemonic::BRV: return "Branch if V";
  case detail::pep10::Mnemonic::BRC: return "Branch if C";
  case detail::pep10::Mnemonic::CALL: return "Call subroutine";
  case detail::pep10::Mnemonic::SCALL: return "System call";
  case detail::pep10::Mnemonic::ADDSP: return "Add to SP";
  case detail::pep10::Mnemonic::SUBSP: return "Subtract from SP";
  case detail::pep10::Mnemonic::ADDA: [[fallthrough]];
  case detail::pep10::Mnemonic::ADDX: return "Add to r";
  case detail::pep10::Mnemonic::SUBA: [[fallthrough]];
  case detail::pep10::Mnemonic::SUBX: return "Subtract from r";
  case detail::pep10::Mnemonic::ANDA: [[fallthrough]];
  case detail::pep10::Mnemonic::ANDX: return "Bitwise And to r";
  case detail::pep10::Mnemonic::ORA: [[fallthrough]];
  case detail::pep10::Mnemonic::ORX: return "Bitwise Or to r";
  case detail::pep10::Mnemonic::XORA: [[fallthrough]];
  case detail::pep10::Mnemonic::XORX: return "Bitwise Exclusive Or to r";
  case detail::pep10::Mnemonic::CPWA: [[fallthrough]];
  case detail::pep10::Mnemonic::CPWX: return "Compare word to r";
  case detail::pep10::Mnemonic::CPBA: [[fallthrough]];
  case detail::pep10::Mnemonic::CPBX: return "Compare byte to r[8:15]";
  case detail::pep10::Mnemonic::LDWA: [[fallthrough]];
  case detail::pep10::Mnemonic::LDWX: return "Load word r from memory";
  case detail::pep10::Mnemonic::LDBA: [[fallthrough]];
  case detail::pep10::Mnemonic::LDBX: return "Load byte r[8:15] from memory";
  case detail::pep10::Mnemonic::STWA: [[fallthrough]];
  case detail::pep10::Mnemonic::STWX: return "Store word r to memory";
  case detail::pep10::Mnemonic::STBA: [[fallthrough]];
  case detail::pep10::Mnemonic::STBX: return "Store byte r[8:15] to memory";
  default: return "Illegal instruction";
  }
}

std::string isa::Pep10::instructionSpecifierWithPlaceholders(Mnemonic mnemonic) {
  using enum detail::pep10::InstructionType;
  uint8_t opcode = static_cast<uint8_t>(mnemonic);
  std::string as_bin = std::bitset<8>(opcode).to_string();
  switch (opcodeLUT[opcode].instr.type) {
  case R_none: as_bin[7] = 'r'; break;
  case A_ix: as_bin[7] = 'a'; break;
  case AAA_all: [[fallthrough]];
  case AAA_i: as_bin[7] = as_bin[6] = as_bin[5] = 'a'; break;
  case RAAA_all: [[fallthrough]];
  case RAAA_noi:
    as_bin[7] = as_bin[6] = as_bin[5] = 'a';
    as_bin[4] = 'r';
    break;
  default: break;
  }
  return as_bin;
}

bool isa::Pep10::requiresAddressingMode(Mnemonic mnemonic) { return isAAAType(mnemonic) || isRAAAType(mnemonic); }

bool isa::Pep10::canElideAddressingMode(Mnemonic mnemonic, AddressingMode addr) {
  return isAType(mnemonic) && addr == AddressingMode::I;
}

std::set<std::string> const &isa::Pep10::legalDirectives() {
  static const auto valid = std::set<std::string>{"ALIGN", "ASCII",  "BLOCK", "BYTE",  "EQUATE",  "EXPORT", "IMPORT",
                                                  "INPUT", "OUTPUT", "ORG",   "SCALL", "SECTION", "WORD"};
  return valid;
}

bool isa::Pep10::isLegalDirective(const std::string &directive) {
  return legalDirectives().contains(bits::to_upper(directive));
}

const std::map<isa::Pep10::AddressingMode, std::string> &isa::Pep10::addressmode_to_string() {
  static const auto r = am_maps().first;
  return r;
}

const std::unordered_map<std::string, isa::Pep10::AddressingMode, pepp::bts::ci_hash, pepp::bts::ci_eq> &
isa::Pep10::string_to_addressmode() {
  static const auto r = am_maps().second;
  return r;
}

const std::map<isa::Pep10::Mnemonic, std::string> &isa::Pep10::mnemonic_to_string() {
  static const auto r = mnemonic_maps().first;
  return r;
}

const std::unordered_map<std::string, isa::Pep10::Mnemonic, pepp::bts::ci_hash, pepp::bts::ci_eq> &
isa::Pep10::string_to_mnemonic() {
  static const auto r = mnemonic_maps().second;
  return r;
}

const std::map<isa::Pep10::Register, std::string> &isa::Pep10::register_to_string() {
  static const auto r = register_maps().first;
  return r;
}

const std::unordered_map<std::string, isa::Pep10::Register, pepp::bts::ci_hash, pepp::bts::ci_eq> &
isa::Pep10::string_to_register() {
  static const auto r = register_maps().second;
  return r;
}
