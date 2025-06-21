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
  else return false;
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

uint8_t pepp::ucode::Pep9ByteBus::register_byte_size(NamedRegisters reg) {
  switch (reg) {
  case NamedRegisters::T1: return 1;
  case NamedRegisters::IR: return 3;
  default: return 2;
  }
}

std::optional<pepp::ucode::Pep9ByteBus::NamedRegisters> pepp::ucode::Pep9ByteBus::parse_register(const QString &name) {
  QStringView v(name);
  return parse_register(v);
}

std::optional<pepp::ucode::Pep9ByteBus::NamedRegisters>
pepp::ucode::Pep9ByteBus::parse_register(const QStringView &name) {
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
  else return false;
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

uint8_t pepp::ucode::Pep9WordBus::register_byte_size(NamedRegisters reg) {
  switch (reg) {
  case NamedRegisters::T1: return 1;
  case NamedRegisters::IR: return 3;
  default: return 2;
  }
}

std::optional<pepp::ucode::Pep9WordBus::NamedRegisters> pepp::ucode::Pep9WordBus::parse_register(const QString &name) {
  QStringView v(name);
  return parse_register(v);
}

std::optional<pepp::ucode::Pep9WordBus::NamedRegisters>
pepp::ucode::Pep9WordBus::parse_register(const QStringView &name) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<NamedRegisters>();
  for (int it = 0; it < meta_enum.keyCount(); it++)
    if (name.compare(meta_enum.key(it), Qt::CaseInsensitive) == 0)
      return static_cast<NamedRegisters>(meta_enum.value(it));
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
  else return false;
}

uint8_t pepp::ucode::Pep9WordBusControl::register_byte_size(NamedRegisters reg) {
  switch (reg) {
  case NamedRegisters::T1: return 1;
  case NamedRegisters::IR: return 3;
  default: return 2;
  }
}

std::optional<pepp::ucode::Pep9WordBusControl::NamedRegisters>
pepp::ucode::Pep9WordBusControl::parse_register(const QString &name) {
  QStringView v(name);
  return parse_register(v);
}

std::optional<pepp::ucode::Pep9WordBusControl::NamedRegisters>
pepp::ucode::Pep9WordBusControl::parse_register(const QStringView &name) {
  static const QMetaEnum meta_enum = QMetaEnum::fromType<NamedRegisters>();
  for (int it = 0; it < meta_enum.keyCount(); it++)
    if (name.compare(meta_enum.key(it), Qt::CaseInsensitive) == 0)
      return static_cast<NamedRegisters>(meta_enum.value(it));
  return std::nullopt;
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
