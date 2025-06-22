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
#include "uarch.hpp"

void pepp::ucode::Pep9ByteBus::Code::set(Signals s, uint8_t value) {
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

uint8_t pepp::ucode::Pep9ByteBus::Code::get(Signals s) const {
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

void pepp::ucode::Pep9ByteBus::CodeWithEnables::enable(Signals s) { enables.set(static_cast<int>(s)); }

bool pepp::ucode::Pep9ByteBus::CodeWithEnables::enabled(Signals s) const { return enables.test(static_cast<int>(s)); }

void pepp::ucode::Pep9ByteBus::CodeWithEnables::clear(Signals s) {
  enables.set(static_cast<int>(s), false);
  code.set(s, 0);
}

void pepp::ucode::Pep9ByteBus::CodeWithEnables::set(Signals s, uint8_t value) {
  enables.set(static_cast<int>(s), true);
  code.set(s, value);
}

uint8_t pepp::ucode::Pep9ByteBus::CodeWithEnables::get(Signals s) const { return code.get(s); }

QString pepp::ucode::Pep9ByteBus::CodeWithEnables::toString() const {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<Signals>();
  QStringList ret;
  QStringList group;
  for (int it = 0; it < meta_enum.keyCount(); it++) {
    auto signal = static_cast<Signals>(meta_enum.value(it));
    if (enabled(signal)) {
      if (is_clock(signal)) group.append(meta_enum.key(it));
      else group.append(QString("%1=%2").arg(meta_enum.key(it), QString::number(get(signal))));
    }
    if (signal == Signals::MDRMux && !group.empty()) {
      ret.append(group.join(", "));
      group.clear();
    }
  }
  if (!group.empty()) ret.append(group.join(", "));
  return ret.join("; ");
}

uint8_t pepp::ucode::Pep9ByteBus::signal_group(Signals s) {
  using enum Signals;
  if (static_cast<int>(s) < static_cast<int>(NCk)) return 0;
  else return 1;
}

bool pepp::ucode::Pep9ByteBus::is_clock(Signals s) {
  using enum Signals;
  if (static_cast<int>(s) >= static_cast<int>(NCk)) return true;
  else return s == Signals::MemRead || s == Signals::MemWrite;
}

uint8_t pepp::ucode::Pep9ByteBus::hidden_register_count() {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<detail::pep9_1byte::HiddenRegisters>();
  return meta_enum.keyCount();
}

std::optional<pepp::ucode::Pep9ByteBus::Signals> pepp::ucode::Pep9ByteBus::parse_signal(const QString &name) {
  QStringView v(name);
  return parse_signal(v);
}

std::optional<pepp::ucode::Pep9ByteBus::Signals> pepp::ucode::Pep9ByteBus::parse_signal(const QStringView &name) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<Signals>();
  for (int it = 0; it < meta_enum.keyCount(); it++)
    if (name.compare(meta_enum.key(it), Qt::CaseInsensitive) == 0) return static_cast<Signals>(meta_enum.value(it));
  return std::nullopt;
}

uint8_t pepp::ucode::Pep9Registers::register_byte_size(NamedRegisters reg) {
  switch (reg) {
  case NamedRegisters::T1: return 1;
  case NamedRegisters::IR: return 3;
  default: return 2;
  }
}

QString pepp::ucode::Pep9Registers::register_name(NamedRegisters reg) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<NamedRegisters>();
  return meta_enum.key(static_cast<int>(reg));
}

QString pepp::ucode::Pep9Registers::csr_name(CSRs reg) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<CSRs>();
  return meta_enum.key(static_cast<int>(reg));
}

std::optional<pepp::ucode::Pep9Registers::CSRs> pepp::ucode::Pep9Registers::parse_csr(const QStringView &name) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<CSRs>();
  for (int it = 0; it < meta_enum.keyCount(); it++)
    if (name.compare(meta_enum.key(it), Qt::CaseInsensitive) == 0) return static_cast<CSRs>(meta_enum.value(it));
  return std::nullopt;
}

std::optional<pepp::ucode::Pep9Registers::CSRs> pepp::ucode::Pep9Registers::parse_csr(const QString &name) {
  QStringView v(name);
  return parse_csr(v);
}

std::optional<pepp::ucode::Pep9Registers::NamedRegisters>
pepp::ucode::Pep9Registers::parse_register(const QString &name) {
  QStringView v(name);
  return parse_register(v);
}

std::optional<pepp::ucode::Pep9Registers::NamedRegisters>
pepp::ucode::Pep9Registers::parse_register(const QStringView &name) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<NamedRegisters>();
  for (int it = 0; it < meta_enum.keyCount(); it++)
    if (name.compare(meta_enum.key(it), Qt::CaseInsensitive) == 0)
      return static_cast<NamedRegisters>(meta_enum.value(it));
  return std::nullopt;
}

void pepp::ucode::Pep9WordBus::Code::set(Signals s, uint8_t value) {
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

uint8_t pepp::ucode::Pep9WordBus::Code::get(Signals s) const {
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

void pepp::ucode::Pep9WordBus::CodeWithEnables::enable(Signals s) { enables.set(static_cast<int>(s)); }

bool pepp::ucode::Pep9WordBus::CodeWithEnables::enabled(Signals s) const { return enables.test(static_cast<int>(s)); }

void pepp::ucode::Pep9WordBus::CodeWithEnables::clear(Signals s) {
  enables.set(static_cast<int>(s), false);
  code.set(s, 0);
}

void pepp::ucode::Pep9WordBus::CodeWithEnables::set(Signals s, uint8_t value) {
  enables.set(static_cast<int>(s), true);
  code.set(s, value);
}

uint8_t pepp::ucode::Pep9WordBus::CodeWithEnables::get(Signals s) const { return code.get(s); }

QString pepp::ucode::Pep9WordBus::CodeWithEnables::toString() const {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<Signals>();
  QStringList ret;
  QStringList group;
  for (int it = 0; it < meta_enum.keyCount(); it++) {
    auto signal = static_cast<Signals>(meta_enum.value(it));
    if (enabled(signal)) {
      if (is_clock(signal)) group.append(meta_enum.key(it));
      else group.append(QString("%1=%2").arg(meta_enum.key(it), QString::number(get(signal))));
    }
    if (signal == Signals::MDREMux && !group.empty()) {
      ret.append(group.join(", "));
      group.clear();
    }
  }
  if (!group.empty()) ret.append(group.join(", "));
  return ret.join("; ");
}

uint8_t pepp::ucode::Pep9WordBus::signal_group(Signals s) {
  using enum Signals;
  if (static_cast<int>(s) < static_cast<int>(NCk)) return 0;
  else return 1;
}

bool pepp::ucode::Pep9WordBus::is_clock(Signals s) {
  using enum Signals;
  if (static_cast<int>(s) >= static_cast<int>(NCk)) return true;
  else return s == Signals::MemRead || s == Signals::MemWrite;
}

uint8_t pepp::ucode::Pep9WordBus::hidden_register_count() {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<detail::pep9_2byte::HiddenRegisters>();
  return meta_enum.keyCount();
}

std::optional<pepp::ucode::Pep9WordBus::Signals> pepp::ucode::Pep9WordBus::parse_signal(const QString &name) {
  QStringView v(name);
  return parse_signal(v);
}

std::optional<pepp::ucode::Pep9WordBus::Signals> pepp::ucode::Pep9WordBus::parse_signal(const QStringView &name) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<Signals>();
  for (int it = 0; it < meta_enum.keyCount(); it++)
    if (name.compare(meta_enum.key(it), Qt::CaseInsensitive) == 0) return static_cast<Signals>(meta_enum.value(it));
  return std::nullopt;
}

uint8_t pepp::ucode::Pep9WordBusControl::signal_group(Signals s) {
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

bool pepp::ucode::Pep9WordBusControl::is_clock(Signals s) {
  using enum Signals;
  if (auto i = static_cast<int>(s); static_cast<int>(NCk) <= i && i <= static_cast<int>(MDRECk)) return true;
  else return s == Signals::MemRead || s == Signals::MemWrite;
}

std::optional<pepp::ucode::Pep9WordBusControl::Signals>
pepp::ucode::Pep9WordBusControl::parse_signal(const QString &name) {
  QStringView v(name);
  return parse_signal(v);
}

std::optional<pepp::ucode::Pep9WordBusControl::Signals>
pepp::ucode::Pep9WordBusControl::parse_signal(const QStringView &name) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<Signals>();
  for (int it = 0; it < meta_enum.keyCount(); it++)
    if (name.compare(meta_enum.key(it), Qt::CaseInsensitive) == 0) return static_cast<Signals>(meta_enum.value(it));
  return std::nullopt;
}

void pepp::ucode::Pep9WordBusControl::Code::set(Signals s, uint8_t value) {
  using enum Signals;
  switch (s) {
  case PreValid: this->PreValid = value; break;
  case BR: this->BR = value; break;
  case TrueT: this->TrueT = value; break;
  case FalseT: this->FalseT = value; break;
  default: Pep9WordBus::Code::set(static_cast<Pep9WordBus::Signals>(static_cast<int>(s)), value);
  }
}

uint8_t pepp::ucode::Pep9WordBusControl::Code::get(Signals s) const {
  using enum Signals;
  switch (s) {
  case PreValid: return this->PreValid;
  case BR: return this->BR;
  case TrueT: return this->TrueT;
  case FalseT: return this->FalseT;
  default: return Pep9WordBus::Code::get(static_cast<Pep9WordBus::Signals>(static_cast<int>(s)));
  }
}

void pepp::ucode::Pep9WordBusControl::CodeWithEnables::enable(Signals s) { enables.set(static_cast<int>(s)); }

bool pepp::ucode::Pep9WordBusControl::CodeWithEnables::enabled(Signals s) const {
  return enables.test(static_cast<int>(s));
}

void pepp::ucode::Pep9WordBusControl::CodeWithEnables::clear(Signals s) {
  enables.set(static_cast<int>(s), false);
  code.set(s, 0);
}

void pepp::ucode::Pep9WordBusControl::CodeWithEnables::set(Signals s, uint8_t value) {
  enables.set(static_cast<int>(s), true);
  code.set(s, value);
}

uint8_t pepp::ucode::Pep9WordBusControl::CodeWithEnables::get(Signals s) const { return code.get(s); }

QString pepp::ucode::Pep9WordBusControl::CodeWithEnables::toString() const {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<Signals>();
  QStringList ret;
  QStringList group;
  for (int it = 0; it < meta_enum.keyCount(); it++) {
    auto signal = static_cast<Signals>(meta_enum.value(it));
    if (signal == Signals::PreValid) continue;

    if (enabled(signal)) {
      if (is_clock(signal)) group.append(meta_enum.key(it));
      else group.append(QString("%1=%2").arg(meta_enum.key(it), QString::number(get(signal))));
    }
    if (auto pv = Signals::PreValid; signal == Signals::MDREMux && enabled(pv))
      group.append(QString("%1=%2").arg(meta_enum.key(static_cast<int>(pv)), QString::number(get(pv))));

    if (signal == Signals::MDREMux && !group.empty()) {
      ret.append(group.join(", "));
      group.clear();
    } else if (signal == Signals::MDRECk && !group.empty()) {
      ret.append(group.join(", "));
      group.clear();
    }
  }
  if (!group.empty()) ret.append(group.join(", "));
  return ret.join("; ");
}

quint8 pepp::ucode::detail::pep9_1byte::computeALU(quint8 fn, quint8 a, quint8 b, bool cin, bool &n, bool &z, bool &v,
                                                   bool &c) {
  quint8 ret = 0;
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
