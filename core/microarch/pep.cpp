/*
 * Copyright (c) 2023-2025 J. Stanley Warford, Matthew McRaven
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
#include "core/microarch/pep.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <vector>

static auto p9register_maps() {
  std::map<pepp::tc::arch::Pep9Registers::NamedRegisters, std::string> reg_to_str;
  std::unordered_map<std::string, pepp::tc::arch::Pep9Registers::NamedRegisters, pepp::bts::ci_hash, pepp::bts::ci_eq>
      str_to_reg;
  auto insert = [](auto &reg_to_str, auto &str_to_reg, pepp::tc::arch::Pep9Registers::NamedRegisters reg,
                   std::string str) {
    reg_to_str[reg] = str;
    str_to_reg[str] = reg;
  };
  using enum pepp::tc::arch::Pep9Registers::NamedRegisters;
  insert(reg_to_str, str_to_reg, A, "A");
  insert(reg_to_str, str_to_reg, X, "X");
  insert(reg_to_str, str_to_reg, SP, "SP");
  insert(reg_to_str, str_to_reg, PC, "PC");
  insert(reg_to_str, str_to_reg, IR, "IR");
  insert(reg_to_str, str_to_reg, T1, "T1");
  insert(reg_to_str, str_to_reg, T2, "T2");
  insert(reg_to_str, str_to_reg, T3, "T3");
  insert(reg_to_str, str_to_reg, T4, "T4");
  insert(reg_to_str, str_to_reg, T5, "T5");
  insert(reg_to_str, str_to_reg, T6, "T6");
  insert(reg_to_str, str_to_reg, M1, "M1");
  insert(reg_to_str, str_to_reg, M2, "M2");
  insert(reg_to_str, str_to_reg, M3, "M3");
  insert(reg_to_str, str_to_reg, M4, "M4");
  insert(reg_to_str, str_to_reg, M5, "M5");
  insert(reg_to_str, str_to_reg, INVALID, "invalid");

  return std::make_pair(reg_to_str, str_to_reg);
}

static auto p9csr_maps() {
  std::map<pepp::tc::arch::Pep9Registers::CSRs, std::string> reg_to_str;
  std::unordered_map<std::string, pepp::tc::arch::Pep9Registers::CSRs, pepp::bts::ci_hash, pepp::bts::ci_eq> str_to_reg;
  auto insert = [](auto &reg_to_str, auto &str_to_reg, pepp::tc::arch::Pep9Registers::CSRs reg, std::string str) {
    reg_to_str[reg] = str;
    str_to_reg[str] = reg;
  };
  using enum pepp::tc::arch::Pep9Registers::CSRs;
  insert(reg_to_str, str_to_reg, N, "N");
  insert(reg_to_str, str_to_reg, Z, "Z");
  insert(reg_to_str, str_to_reg, V, "V");
  insert(reg_to_str, str_to_reg, C, "C");
  insert(reg_to_str, str_to_reg, S, "S");

  return std::make_pair(reg_to_str, str_to_reg);
}

static auto p9bytesignals_maps() {
  std::map<pepp::tc::arch::Pep9ByteBus::Signals, std::string> reg_to_str;
  std::unordered_map<std::string, pepp::tc::arch::Pep9ByteBus::Signals, pepp::bts::ci_hash, pepp::bts::ci_eq>
      str_to_reg;
  auto insert = [](auto &reg_to_str, auto &str_to_reg, pepp::tc::arch::Pep9ByteBus::Signals reg, std::string str) {
    reg_to_str[reg] = str;
    str_to_reg[str] = reg;
  };

  using enum pepp::tc::arch::Pep9ByteBus::Signals;
  insert(reg_to_str, str_to_reg, MemRead, "MemRead");
  insert(reg_to_str, str_to_reg, MemWrite, "MemWrite");
  insert(reg_to_str, str_to_reg, A, "A");
  insert(reg_to_str, str_to_reg, B, "B");
  insert(reg_to_str, str_to_reg, AMux, "AMux");
  insert(reg_to_str, str_to_reg, ALU, "ALU");
  insert(reg_to_str, str_to_reg, CSMux, "CSMux");
  insert(reg_to_str, str_to_reg, AndZ, "AndZ");
  insert(reg_to_str, str_to_reg, CMux, "CMux");
  insert(reg_to_str, str_to_reg, C, "C");
  insert(reg_to_str, str_to_reg, MDRMux, "MDRMux");
  insert(reg_to_str, str_to_reg, NCk, "NCk");
  insert(reg_to_str, str_to_reg, ZCk, "ZCk");
  insert(reg_to_str, str_to_reg, VCk, "VCk");
  insert(reg_to_str, str_to_reg, CCk, "CCk");
  insert(reg_to_str, str_to_reg, SCk, "SCk");
  insert(reg_to_str, str_to_reg, MARCk, "MARCk");
  insert(reg_to_str, str_to_reg, LoadCk, "LoadCk");
  insert(reg_to_str, str_to_reg, MDRCk, "MDRCk");

  return std::make_pair(reg_to_str, str_to_reg);
}

static auto p9bytehidden_maps() {
  std::map<pepp::tc::arch::Pep9ByteBus::HiddenRegisters, std::string> reg_to_str;
  std::unordered_map<std::string, pepp::tc::arch::Pep9ByteBus::HiddenRegisters, pepp::bts::ci_hash, pepp::bts::ci_eq>
      str_to_reg;
  auto insert = [](auto &reg_to_str, auto &str_to_reg, pepp::tc::arch::Pep9ByteBus::HiddenRegisters reg,
                   std::string str) {
    reg_to_str[reg] = str;
    str_to_reg[str] = reg;
  };
  using enum pepp::tc::arch::Pep9ByteBus::HiddenRegisters;
  insert(reg_to_str, str_to_reg, MARA, "MARA");
  insert(reg_to_str, str_to_reg, MARB, "MARB");
  insert(reg_to_str, str_to_reg, MDR, "MDR");

  return std::make_pair(reg_to_str, str_to_reg);
}

static auto p9wordsignals_maps() {
  std::map<pepp::tc::arch::Pep9WordBus::Signals, std::string> reg_to_str;
  std::unordered_map<std::string, pepp::tc::arch::Pep9WordBus::Signals, pepp::bts::ci_hash, pepp::bts::ci_eq>
      str_to_reg;
  auto insert = [](auto &reg_to_str, auto &str_to_reg, pepp::tc::arch::Pep9WordBus::Signals reg, std::string str) {
    reg_to_str[reg] = str;
    str_to_reg[str] = reg;
  };

  using enum pepp::tc::arch::Pep9WordBus::Signals;
  insert(reg_to_str, str_to_reg, MemRead, "MemRead");
  insert(reg_to_str, str_to_reg, MemWrite, "MemWrite");
  insert(reg_to_str, str_to_reg, A, "A");
  insert(reg_to_str, str_to_reg, B, "B");
  insert(reg_to_str, str_to_reg, EOMux, "EOMux");
  insert(reg_to_str, str_to_reg, MARMux, "MARMux");
  insert(reg_to_str, str_to_reg, AMux, "AMux");
  insert(reg_to_str, str_to_reg, ALU, "ALU");
  insert(reg_to_str, str_to_reg, CSMux, "CSMux");
  insert(reg_to_str, str_to_reg, AndZ, "AndZ");
  insert(reg_to_str, str_to_reg, CMux, "CMux");
  insert(reg_to_str, str_to_reg, C, "C");
  insert(reg_to_str, str_to_reg, MDROMux, "MDROMux");
  insert(reg_to_str, str_to_reg, MDREMux, "MDREMux");
  insert(reg_to_str, str_to_reg, NCk, "NCk");
  insert(reg_to_str, str_to_reg, ZCk, "ZCk");
  insert(reg_to_str, str_to_reg, VCk, "VCk");
  insert(reg_to_str, str_to_reg, CCk, "CCk");
  insert(reg_to_str, str_to_reg, SCk, "SCk");
  insert(reg_to_str, str_to_reg, MARCk, "MARCk");
  insert(reg_to_str, str_to_reg, LoadCk, "LoadCk");
  insert(reg_to_str, str_to_reg, MDROCk, "MDROCk");
  insert(reg_to_str, str_to_reg, MDRECk, "MDRECk");

  return std::make_pair(reg_to_str, str_to_reg);
}

static auto p9wordhidden_maps() {
  std::map<pepp::tc::arch::Pep9WordBus::HiddenRegisters, std::string> reg_to_str;
  std::unordered_map<std::string, pepp::tc::arch::Pep9WordBus::HiddenRegisters, pepp::bts::ci_hash, pepp::bts::ci_eq>
      str_to_reg;
  auto insert = [](auto &reg_to_str, auto &str_to_reg, pepp::tc::arch::Pep9WordBus::HiddenRegisters reg,
                   std::string str) {
    reg_to_str[reg] = str;
    str_to_reg[str] = reg;
  };
  using enum pepp::tc::arch::Pep9WordBus::HiddenRegisters;
  insert(reg_to_str, str_to_reg, MARA, "MARA");
  insert(reg_to_str, str_to_reg, MARB, "MARB");
  insert(reg_to_str, str_to_reg, MDRE, "MDRE");
  insert(reg_to_str, str_to_reg, MDRO, "MDRO");

  return std::make_pair(reg_to_str, str_to_reg);
}

static auto p9ctrlsignals_maps() {
  std::map<pepp::tc::arch::Pep9WordBusControl::Signals, std::string> reg_to_str;
  std::unordered_map<std::string, pepp::tc::arch::Pep9WordBusControl::Signals, pepp::bts::ci_hash, pepp::bts::ci_eq>
      str_to_reg;
  auto insert = [](auto &reg_to_str, auto &str_to_reg, pepp::tc::arch::Pep9WordBusControl::Signals reg,
                   std::string str) {
    reg_to_str[reg] = str;
    str_to_reg[str] = reg;
  };

  using enum pepp::tc::arch::Pep9WordBusControl::Signals;
  insert(reg_to_str, str_to_reg, MemRead, "MemRead");
  insert(reg_to_str, str_to_reg, MemWrite, "MemWrite");
  insert(reg_to_str, str_to_reg, A, "A");
  insert(reg_to_str, str_to_reg, B, "B");
  insert(reg_to_str, str_to_reg, EOMux, "EOMux");
  insert(reg_to_str, str_to_reg, MARMux, "MARMux");
  insert(reg_to_str, str_to_reg, AMux, "AMux");
  insert(reg_to_str, str_to_reg, ALU, "ALU");
  insert(reg_to_str, str_to_reg, CSMux, "CSMux");
  insert(reg_to_str, str_to_reg, AndZ, "AndZ");
  insert(reg_to_str, str_to_reg, CMux, "CMux");
  insert(reg_to_str, str_to_reg, C, "C");
  insert(reg_to_str, str_to_reg, MDROMux, "MDROMux");
  insert(reg_to_str, str_to_reg, MDREMux, "MDREMux");
  insert(reg_to_str, str_to_reg, NCk, "NCk");
  insert(reg_to_str, str_to_reg, ZCk, "ZCk");
  insert(reg_to_str, str_to_reg, VCk, "VCk");
  insert(reg_to_str, str_to_reg, CCk, "CCk");
  insert(reg_to_str, str_to_reg, SCk, "SCk");
  insert(reg_to_str, str_to_reg, MARCk, "MARCk");
  insert(reg_to_str, str_to_reg, LoadCk, "LoadCk");
  insert(reg_to_str, str_to_reg, MDROCk, "MDROCk");
  insert(reg_to_str, str_to_reg, MDRECk, "MDRECk");
  insert(reg_to_str, str_to_reg, PreValid, "PreValid");
  insert(reg_to_str, str_to_reg, BR, "BR");
  insert(reg_to_str, str_to_reg, TrueT, "TrueT");
  insert(reg_to_str, str_to_reg, FalseT, "FalseT");

  return std::make_pair(reg_to_str, str_to_reg);
}

void pepp::tc::arch::Pep9ByteBus::Code::set(Signals s, uint8_t value) {
  using enum detail::pep9_1byte::Signals;
  switch (s) {
  case MemRead: this->MemRead = value; break;
  case MemWrite: this->MemWrite = value; break;
  case A: this->A = value; break;
  case B: this->B = value; break;
  case AMux: this->AMux = value; break;
  case ALU: this->ALU = value; break;
  case CSMux: this->CSMux = value; break;
  case AndZ: this->AndZ = value; break;
  case CMux: this->CMux = value; break;
  case C: this->C = value; break;
  case MDRMux: this->MDRMux = value; break;
  case NCk: this->NCk = value; break;
  case ZCk: this->ZCk = value; break;
  case VCk: this->VCk = value; break;
  case CCk: this->CCk = value; break;
  case SCk: this->SCk = value; break;
  case MARCk: this->MARCk = value; break;
  case LoadCk: this->LoadCk = value; break;
  case MDRCk: this->MDRCk = value; break;
  }
}

uint8_t pepp::tc::arch::Pep9ByteBus::Code::get(Signals s) const {
  using enum detail::pep9_1byte::Signals;
  switch (s) {
  case MemRead: return this->MemRead;
  case MemWrite: return this->MemWrite;
  case Signals::A: return this->A;
  case B: return this->B;
  case AMux: return this->AMux;
  case ALU: return this->ALU;
  case CSMux: return this->CSMux;
  case AndZ: return this->AndZ;
  case CMux: return this->CMux;
  case C: return this->C;
  case MDRMux: return this->MDRMux;
  case NCk: return this->NCk;
  case ZCk: return this->ZCk;
  case VCk: return this->VCk;
  case CCk: return this->CCk;
  case SCk: return this->SCk;
  case MARCk: return this->MARCk;
  case LoadCk: return this->LoadCk;
  case MDRCk: return this->MDRCk;
  }
  return 0;
}

void pepp::tc::arch::Pep9ByteBus::CodeWithEnables::enable(Signals s) { enables.set(static_cast<int>(s)); }

bool pepp::tc::arch::Pep9ByteBus::CodeWithEnables::enabled(Signals s) const {
  return enables.test(static_cast<int>(s));
}

void pepp::tc::arch::Pep9ByteBus::CodeWithEnables::clear(Signals s) {
  enables.set(static_cast<int>(s), false);
  code.set(s, 0);
}

void pepp::tc::arch::Pep9ByteBus::CodeWithEnables::set(Signals s, uint8_t value) {
  enables.set(static_cast<int>(s), true);
  code.set(s, value);
}

uint8_t pepp::tc::arch::Pep9ByteBus::CodeWithEnables::get(Signals s) const { return code.get(s); }

std::string pepp::tc::arch::Pep9ByteBus::CodeWithEnables::toString() const {
  auto signals = signal_to_string();
  std::vector<std::string> ret, group;
  for (const auto &[signal, name] : signals) {
    if (enabled(signal)) {
      if (is_clock(signal)) group.emplace_back(name);
      else group.emplace_back(fmt::format("{}={}", name, get(signal)));
    }
    if (signal == Signals::MDRMux && !group.empty()) {
      ret.emplace_back(fmt::format("{}", fmt::join(group, ", ")));
      group.clear();
    }
  }
  if (!group.empty()) ret.emplace_back(fmt::format("{}", fmt::join(group, ", ")));
  return fmt::format("{}", fmt::join(ret, "; "));
}

uint8_t pepp::tc::arch::Pep9ByteBus::signal_group(Signals s) {
  using enum Signals;
  if (static_cast<int>(s) < static_cast<int>(NCk)) return 0;
  else return 1;
}

bool pepp::tc::arch::Pep9ByteBus::is_clock(Signals s) {
  using enum Signals;
  if (static_cast<int>(s) >= static_cast<int>(NCk)) return true;
  else return s == Signals::MemRead || s == Signals::MemWrite;
}

uint8_t pepp::tc::arch::Pep9ByteBus::hidden_register_count() { return hiddenregister_to_string().size(); }

const std::unordered_map<std::string, pepp::tc::arch::Pep9ByteBus::Signals, pepp::bts::ci_hash, pepp::bts::ci_eq> &
pepp::tc::arch::Pep9ByteBus::string_to_signal() {
  static const auto ret = p9bytesignals_maps().second;
  return ret;
}

const std::map<pepp::tc::arch::Pep9ByteBus::Signals, std::string> &pepp::tc::arch::Pep9ByteBus::signal_to_string() {
  static const auto ret = p9bytesignals_maps().first;
  return ret;
}

const std::unordered_map<std::string, pepp::tc::arch::Pep9ByteBus::HiddenRegisters, pepp::bts::ci_hash,
                         pepp::bts::ci_eq> &
pepp::tc::arch::Pep9ByteBus::string_to_hiddenregister() {
  static const auto ret = p9bytehidden_maps().second;
  return ret;
}

const std::map<pepp::tc::arch::Pep9ByteBus::HiddenRegisters, std::string> &
pepp::tc::arch::Pep9ByteBus::hiddenregister_to_string() {
  static const auto ret = p9bytehidden_maps().first;
  return ret;
}

std::optional<pepp::tc::arch::Pep9ByteBus::Signals> pepp::tc::arch::Pep9ByteBus::parse_signal(const std::string &name) {
  std::string_view v(name);
  return parse_signal(v);
}

std::optional<pepp::tc::arch::Pep9ByteBus::Signals>
pepp::tc::arch::Pep9ByteBus::parse_signal(const std::string_view &name) {
  auto strs = string_to_signal();
  auto it = strs.find(name);
  if (it != strs.end()) return it->second;
  return std::nullopt;
}

uint8_t pepp::tc::arch::Pep9Registers::register_byte_size(NamedRegisters reg) {
  switch (reg) {
  case NamedRegisters::T1: return 1;
  case NamedRegisters::IR: return 3;
  default: return 2;
  }
}

std::string pepp::tc::arch::Pep9Registers::register_name(NamedRegisters reg) {
  return namedregister_to_string().at(reg);
}

std::string pepp::tc::arch::Pep9Registers::csr_name(CSRs reg) { return csr_to_string().at(reg); }

const std::map<pepp::tc::arch::Pep9Registers::NamedRegisters, std::string> &
pepp::tc::arch::Pep9Registers::namedregister_to_string() {
  static const auto ret = p9register_maps().first;
  return ret;
}

const std::unordered_map<std::string, pepp::tc::arch::Pep9Registers::NamedRegisters, pepp::bts::ci_hash,
                         pepp::bts::ci_eq> &
pepp::tc::arch::Pep9Registers::string_to_namedregister() {
  static const auto ret = p9register_maps().second;
  return ret;
}

const std::map<pepp::tc::arch::Pep9Registers::CSRs, std::string> &pepp::tc::arch::Pep9Registers::csr_to_string() {
  static const auto ret = p9csr_maps().first;
  return ret;
}

const std::unordered_map<std::string, pepp::tc::arch::Pep9Registers::CSRs, pepp::bts::ci_hash, pepp::bts::ci_eq> &
pepp::tc::arch::Pep9Registers::string_to_csr() {
  static const auto ret = p9csr_maps().second;
  return ret;
}

std::optional<pepp::tc::arch::Pep9Registers::CSRs>
pepp::tc::arch::Pep9Registers::parse_csr(const std::string_view &name) {
  auto strs = string_to_csr();
  auto it = strs.find(name);
  if (it != strs.end()) return it->second;
  return std::nullopt;
}

std::optional<pepp::tc::arch::Pep9Registers::CSRs> pepp::tc::arch::Pep9Registers::parse_csr(const std::string &name) {
  std::string_view v(name);
  return parse_csr(v);
}

std::optional<pepp::tc::arch::Pep9Registers::NamedRegisters>
pepp::tc::arch::Pep9Registers::parse_register(const std::string_view &name) {
  auto strs = string_to_namedregister();
  auto it = strs.find(name);
  if (it != strs.end()) return it->second;
  return std::nullopt;
}

std::optional<pepp::tc::arch::Pep9Registers::NamedRegisters>
pepp::tc::arch::Pep9Registers::parse_register(const std::string &name) {
  std::string_view v(name);
  return parse_register(v);
}

void pepp::tc::arch::Pep9WordBus::Code::set(Signals s, uint8_t value) {
  using enum detail::pep9_2byte::Signals;
  switch (s) {
  case MemRead: this->MemRead = value; break;
  case MemWrite: this->MemWrite = value; break;
  case A: this->A = value; break;
  case B: this->B = value; break;
  case EOMux: this->EOMux = value; break;
  case MARMux: this->MARMux = value; break;
  case AMux: this->AMux = value; break;
  case ALU: this->ALU = value; break;
  case CSMux: this->CSMux = value; break;
  case AndZ: this->AndZ = value; break;
  case CMux: this->CMux = value; break;
  case C: this->C = value; break;
  case MDROMux: this->MDROMux = value; break;
  case MDREMux: this->MDREMux = value; break;
  case NCk: this->NCk = value; break;
  case ZCk: this->ZCk = value; break;
  case VCk: this->VCk = value; break;
  case CCk: this->CCk = value; break;
  case SCk: this->SCk = value; break;
  case MARCk: this->MARCk = value; break;
  case LoadCk: this->LoadCk = value; break;
  case MDROCk: this->MDROCk = value; break;
  case MDRECk: this->MDRECk = value; break;
  }
}

uint8_t pepp::tc::arch::Pep9WordBus::Code::get(Signals s) const {
  using enum detail::pep9_2byte::Signals;
  switch (s) {
  case MemRead: return this->MemRead;
  case MemWrite: return this->MemWrite;
  case Signals::A: return this->A;
  case B: return this->B;
  case EOMux: return this->EOMux;
  case MARMux: return this->MARMux;
  case AMux: return this->AMux;
  case ALU: return this->ALU;
  case CSMux: return this->CSMux;
  case AndZ: return this->AndZ;
  case CMux: return this->CMux;
  case C: return this->C;
  case MDROMux: return this->MDROMux;
  case MDREMux: return this->MDREMux;
  case NCk: return this->NCk;
  case ZCk: return this->ZCk;
  case VCk: return this->VCk;
  case CCk: return this->CCk;
  case SCk: return this->SCk;
  case MARCk: return this->MARCk;
  case LoadCk: return this->LoadCk;
  case MDROCk: return this->MDROCk;
  case MDRECk: return this->MDRECk;
  }
  return 0;
}

void pepp::tc::arch::Pep9WordBus::CodeWithEnables::enable(Signals s) { enables.set(static_cast<int>(s)); }

bool pepp::tc::arch::Pep9WordBus::CodeWithEnables::enabled(Signals s) const {
  return enables.test(static_cast<int>(s));
}

void pepp::tc::arch::Pep9WordBus::CodeWithEnables::clear(Signals s) {
  enables.set(static_cast<int>(s), false);
  code.set(s, 0);
}

void pepp::tc::arch::Pep9WordBus::CodeWithEnables::set(Signals s, uint8_t value) {
  enables.set(static_cast<int>(s), true);
  code.set(s, value);
}

uint8_t pepp::tc::arch::Pep9WordBus::CodeWithEnables::get(Signals s) const { return code.get(s); }

std::string pepp::tc::arch::Pep9WordBus::CodeWithEnables::toString() const {
  auto signals = signal_to_string();
  std::vector<std::string> ret, group;
  for (const auto &[signal, name] : signals) {
    if (enabled(signal)) {
      if (is_clock(signal)) group.emplace_back(name);
      else group.emplace_back(fmt::format("{}={}", name, get(signal)));
    }
    if (signal == Signals::MDREMux && !group.empty()) {
      ret.emplace_back(fmt::format("{}", fmt::join(group, ", ")));
      group.clear();
    }
  }
  if (!group.empty()) ret.emplace_back(fmt::format("{}", fmt::join(group, ", ")));
  return fmt::format("{}", fmt::join(ret, "; "));
}

uint8_t pepp::tc::arch::Pep9WordBus::signal_group(Signals s) {
  using enum Signals;
  if (static_cast<int>(s) < static_cast<int>(NCk)) return 0;
  else return 1;
}

bool pepp::tc::arch::Pep9WordBus::is_clock(Signals s) {
  using enum Signals;
  if (static_cast<int>(s) >= static_cast<int>(NCk)) return true;
  else return s == Signals::MemRead || s == Signals::MemWrite;
}

uint8_t pepp::tc::arch::Pep9WordBus::hidden_register_count() { return hiddenregister_to_string().size(); }

const std::unordered_map<std::string, pepp::tc::arch::Pep9WordBus::Signals, pepp::bts::ci_hash, pepp::bts::ci_eq> &
pepp::tc::arch::Pep9WordBus::string_to_signal() {
  static const auto ret = p9wordsignals_maps().second;
  return ret;
}

const std::map<pepp::tc::arch::Pep9WordBus::Signals, std::string> &pepp::tc::arch::Pep9WordBus::signal_to_string() {
  static const auto ret = p9wordsignals_maps().first;
  return ret;
}

const std::unordered_map<std::string, pepp::tc::arch::Pep9WordBus::HiddenRegisters, pepp::bts::ci_hash,
                         pepp::bts::ci_eq> &
pepp::tc::arch::Pep9WordBus::string_to_hiddenregister() {
  static const auto ret = p9wordhidden_maps().second;
  return ret;
}

const std::map<pepp::tc::arch::Pep9WordBus::HiddenRegisters, std::string> &
pepp::tc::arch::Pep9WordBus::hiddenregister_to_string() {
  static const auto ret = p9wordhidden_maps().first;
  return ret;
}

std::optional<pepp::tc::arch::Pep9WordBus::Signals> pepp::tc::arch::Pep9WordBus::parse_signal(const std::string &name) {
  std::string_view v(name);
  return parse_signal(v);
}

std::optional<pepp::tc::arch::Pep9WordBus::Signals>
pepp::tc::arch::Pep9WordBus::parse_signal(const std::string_view &name) {
  auto strs = string_to_signal();
  auto it = strs.find(name);
  if (it != strs.end()) return it->second;
  return std::nullopt;
}

uint8_t pepp::tc::arch::Pep9WordBusControl::signal_group(Signals s) {
  using enum Signals;
  switch (s) {
  case PreValid: return 0;
  case BR: [[fallthrough]];
  case TrueT: [[fallthrough]];
  case FalseT: return 2;
  default: break;
  }

  if (static_cast<int>(s) < static_cast<int>(NCk)) return 0;
  else return 1;
}

bool pepp::tc::arch::Pep9WordBusControl::is_clock(Signals s) {
  using enum Signals;
  if (auto i = static_cast<int>(s); static_cast<int>(NCk) <= i && i <= static_cast<int>(MDRECk)) return true;
  else return s == Signals::MemRead || s == Signals::MemWrite;
}

const std::unordered_map<std::string, pepp::tc::arch::Pep9WordBusControl::Signals, pepp::bts::ci_hash,
                         pepp::bts::ci_eq> &
pepp::tc::arch::Pep9WordBusControl::string_to_signal() {
  static const auto ret = p9ctrlsignals_maps().second;
  return ret;
}

const std::map<pepp::tc::arch::Pep9WordBusControl::Signals, std::string> &
pepp::tc::arch::Pep9WordBusControl::signal_to_string() {
  static const auto ret = p9ctrlsignals_maps().first;
  return ret;
}

std::optional<pepp::tc::arch::Pep9WordBusControl::Signals>
pepp::tc::arch::Pep9WordBusControl::parse_signal(const std::string &name) {
  std::string_view v(name);
  return parse_signal(v);
}

std::optional<pepp::tc::arch::Pep9WordBusControl::Signals>
pepp::tc::arch::Pep9WordBusControl::parse_signal(const std::string_view &name) {
  auto strs = string_to_signal();
  auto it = strs.find(name);
  if (it != strs.end()) return it->second;
  return std::nullopt;
}

void pepp::tc::arch::Pep9WordBusControl::Code::set(Signals s, uint8_t value) {
  using enum Signals;
  switch (s) {
  case PreValid: this->PreValid = value; break;
  case BR: this->BR = value; break;
  case TrueT: this->TrueT = value; break;
  case FalseT: this->FalseT = value; break;
  default: Pep9WordBus::Code::set(static_cast<Pep9WordBus::Signals>(static_cast<int>(s)), value);
  }
}

uint8_t pepp::tc::arch::Pep9WordBusControl::Code::get(Signals s) const {
  using enum Signals;
  switch (s) {
  case PreValid: return this->PreValid;
  case BR: return this->BR;
  case TrueT: return this->TrueT;
  case FalseT: return this->FalseT;
  default: return Pep9WordBus::Code::get(static_cast<Pep9WordBus::Signals>(static_cast<int>(s)));
  }
}

void pepp::tc::arch::Pep9WordBusControl::CodeWithEnables::enable(Signals s) { enables.set(static_cast<int>(s)); }

bool pepp::tc::arch::Pep9WordBusControl::CodeWithEnables::enabled(Signals s) const {
  return enables.test(static_cast<int>(s));
}

void pepp::tc::arch::Pep9WordBusControl::CodeWithEnables::clear(Signals s) {
  enables.set(static_cast<int>(s), false);
  code.set(s, 0);
}

void pepp::tc::arch::Pep9WordBusControl::CodeWithEnables::set(Signals s, uint8_t value) {
  enables.set(static_cast<int>(s), true);
  code.set(s, value);
}

uint8_t pepp::tc::arch::Pep9WordBusControl::CodeWithEnables::get(Signals s) const { return code.get(s); }

std::string pepp::tc::arch::Pep9WordBusControl::CodeWithEnables::toString() const {

  auto signals = signal_to_string();
  std::vector<std::string> ret, group;
  for (const auto &[signal, name] : signals) {
    if (signal == Signals::PreValid) continue;

    if (enabled(signal)) {
      if (is_clock(signal)) group.emplace_back(name);
      else group.emplace_back(fmt::format("{}={}", name, get(signal)));
    }
    if (auto pv = Signals::PreValid; signal == Signals::MDREMux && enabled(pv))
      group.emplace_back(fmt::format("PreValid={}", get(pv)));
    else if (signal == Signals::MDREMux && !group.empty()) {
      ret.emplace_back(fmt::format("{}", fmt::join(group, ", ")));
      group.clear();
    } else if (signal == Signals::MDRECk && !group.empty()) {
      ret.emplace_back(fmt::format("{}", fmt::join(group, ", ")));
      group.clear();
    }
  }
  if (!group.empty()) ret.emplace_back(fmt::format("{}", fmt::join(group, ", ")));
  return fmt::format("{}", fmt::join(ret, "; "));
}

uint8_t pepp::tc::arch::detail::pep9_1byte::computeALU(uint8_t fn, uint8_t a, uint8_t b, bool cin, bool &n, bool &z,
                                                       bool &v, bool &c) {
  uint8_t ret = 0;
  // Common case, saves many lines of code.
  v = c = false;
  // switch case over 16 functions (0-indexed)
  switch (static_cast<ALUFunc>(fn)) {
  case ALUFunc::A: // A
    ret = a;
    break;
    // Re-arrange these functions to avoid duplicate math.
  case ALUFunc::AB_plus: // A plus B
    cin = 0;
    // Makes this easier to read, sorry everyone.
    goto _case2;
  case ALUFunc::ANotB1_plus: // A plus ~B plus 1
    cin = 1;
    [[fallthrough]];
  case ALUFunc::ANotBCin_plus: // A plus ~B plus Cin
    b = ~b;
    [[fallthrough]];
  case ALUFunc::ABCin_plus: // A plus B plus Cin
  _case2:
    ret = a + b + cin;
    v = ((a ^ ret) & (b ^ ret)) & 0x80; // overflow if sign bits of a and b are the same, but different from result
    c = ret < a || ret < b;             // carry if result is less than either operand
    break;
  case ALUFunc::AB_AND: // A AND B
    ret = a & b;
    break;
  case ALUFunc::AB_NAND: // A NAND B
    ret = !(a & b);
    break;
  case ALUFunc::AB_OR: // A OR B
    ret = a | b;
    break;
  case ALUFunc::AB_NOR: // A NOR B
    ret = !(a | b);
    break;
  case ALUFunc::AB_XOR: // A XOR B
    ret = a ^ b;
    break;
  case ALUFunc::NegA: // ~ A
    ret = ~a;
    break;
  case ALUFunc::A_ASL: // ASL A
    ret = a << 1;
    c = a & 0x80;
    v = ((a << 1) ^ a) & 0x80; // overflow if a[0] != a[1]
    break;
  case ALUFunc::A_ROL: // ROL A
    ret = a << 1 | (cin ? 1 : 0);
    c = a & 0x80;
    break;
  case ALUFunc::A_ASR: // ASR A
    cin = a & 128;
    [[fallthrough]];
  case ALUFunc::A_ROR: // ROR A
    ret = (a >> 1) | (cin ? 0x80 : 0);
    c = a & 1;
  case ALUFunc::Zero: // 0
    n = a & 0x8;
    z = a & 0x4;
    v = a & 0x2;
    c = a & 0x1;
    return 0;
  default: throw std::logic_error("Illegal function code");
  }
  n = ret & 0x80;
  z = ret == 0;
  return ret;
}
