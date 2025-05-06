#pragma once
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

#include <QMetaEnum>
#include <QString>

namespace isa::detail {

template <typename Mnemonic> quint8 opcode(Mnemonic mnemonic) { return static_cast<quint8>(mnemonic); }

template <typename AddressingMode, typename Mnemonic> quint8 opcode(Mnemonic mnemonic, AddressingMode addr) {
  using M = Mnemonic;
  using AM = AddressingMode;
  auto base = opcode(mnemonic);
  // TODO: Look up instruction type instead of doing opcode math.
  if (base >= static_cast<quint8>(M::BR) && base <= static_cast<quint8>(M::CALL)) return base | (addr == AM::X ? 1 : 0);
  static const char *const e = "Invalid ADDR mode";
  switch (addr) {
  case AM::NONE: qCritical(e); throw std::logic_error(e);
  case AM::I: return base | 0x0;
  case AM::D: return base | 0x1;
  case AM::N: return base | 0x2;
  case AM::S: return base | 0x3;
  case AM::SF: return base | 0x4;
  case AM::X: return base | 0x5;
  case AM::SX: return base | 0x6;
  case AM::SFX: return base | 0x7;
  case AM::ALL: qCritical(e); throw std::logic_error(e);
  case AM::INVALID: qCritical(e); throw std::logic_error(e);
  }
  static const char *const e2 = "Unreachable";
  qCritical(e2);
  throw std::logic_error(e2);
}

template <typename AddressingMode> AddressingMode parseAddressingMode(const QString &addr) {
  using AM = AddressingMode;
  bool ok = true;
  auto ret = (AM)QMetaEnum::fromType<AddressingMode>().keyToValue(addr.toUpper().toUtf8().data(), &ok);
  if (!ok || ret == AM::ALL || ret == AM::NONE) return AM::INVALID;
  else return ret;
}

template <typename Mnemonic> Mnemonic parseMnemonic(const QString &mnemonic) {
  bool ok = true;
  auto ret = QMetaEnum::fromType<Mnemonic>().keyToValue(mnemonic.toUpper().toUtf8().data(), &ok);
  if (!ok) return Mnemonic::INVALID;
  else return (Mnemonic)ret;
}

template <typename Mnemonic> QString stringMnemonic(Mnemonic mnemonic) {
  return QString(QMetaEnum::fromType<Mnemonic>().valueToKey((int)mnemonic)).toUpper();
}

template <typename AddressingMode> QString stringAddr(AddressingMode addr) {
  return QString(QMetaEnum::fromType<AddressingMode>().valueToKey((int)addr)).toLower();
}

template <typename Mnemonic> bool isStore(Mnemonic mnemonic) {
  using M = Mnemonic;
  if (mnemonic == M::STBA || mnemonic == M::STWA || mnemonic == M::STBX || mnemonic == M::STWX) return true;
  else return false;
}
} // namespace isa::detail
