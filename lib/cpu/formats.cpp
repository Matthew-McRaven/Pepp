/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
#include "formats.hpp"
#include "utils/bits/mask.hpp"
using namespace Qt::StringLiterals;

TextFormatter::TextFormatter(QString value) : _value(value) {}

QString TextFormatter::format() const { return _value; }

QString TextFormatter::format(quint8 byteCount) const { return _value; }

bool TextFormatter::readOnly() const { return true; }

qsizetype TextFormatter::length() const { return _value.length() + 2; }

qsizetype TextFormatter::length(quint8 byteCount) const { return _value.length() + 2; }

HexFormatter::HexFormatter(std::function<uint64_t()> fn, uint16_t byteCount)
    : _fn(fn), _bytes(byteCount), _mask(bits::mask(byteCount)) {}

QString HexFormatter::format() const { return "0x" + u"%1"_s.arg(_mask & _fn(), _bytes * 2, 16, QChar('0')).toUpper(); }

QString HexFormatter::format(quint8 byteCount) const {
  return "0x" + u"%1"_s.arg(bits::mask(byteCount) & _fn(), byteCount * 2, 16, QChar('0')).toUpper();
}

bool HexFormatter::readOnly() const { return false; }

qsizetype HexFormatter::length() const { return length(_bytes); }

qsizetype HexFormatter::length(quint8 byteCount) const { return 2 * byteCount + 2; }

UnsignedDecFormatter::UnsignedDecFormatter(std::function<uint64_t()> fn, uint16_t byteCount)
    : _fn(fn), _bytes(byteCount), _mask(bits::mask(byteCount)), _len(byteCount) {}

QString UnsignedDecFormatter::format() const { return QString::number(_mask & _fn()); }

QString UnsignedDecFormatter::format(quint8 byteCount) const { return QString::number(bits::mask(byteCount) & _fn()); }

bool UnsignedDecFormatter::readOnly() const { return false; }

qsizetype UnsignedDecFormatter::length() const { return _len; }

qsizetype UnsignedDecFormatter::length(quint8 byteCount) const { return digits(byteCount); }

uint16_t UnsignedDecFormatter::digits(quint8 byteCount) { return std::ceil(std::log10(std::pow(2, 8 * byteCount))); }

SignedDecFormatter::SignedDecFormatter(std::function<int64_t()> fn, uint16_t byteCount)
    : _fn(fn), _bytes(byteCount), _mask(bits::mask(byteCount)), _len(digits(byteCount) + 1) {}

QString SignedDecFormatter::format() const {
  if (int64_t v = _fn(); v & (1 << (_bytes * 8 - 1))) {
    int64_t bits = ~(_mask & ~v); // Bit-math turns into unsigned int, we explicitly need signed.
    // Limit V to the range expressable with an N-bit unsigned int before restoring the sign.
    return QString::number(bits);
  }
  return QString::number(_mask & _fn());
}

QString SignedDecFormatter::format(quint8 byteCount) const {
  if (int64_t v = _fn(); v & (1 << (_bytes * 8 - 1))) {
    int64_t bits = ~(bits::mask(byteCount) & ~v);
    // Limit V to the range expressable with an N-bit unsigned int before restoring the sign.
    return QString::number(bits);
  }
  return QString::number(bits::mask(byteCount) & _fn());
}

bool SignedDecFormatter::readOnly() const { return false; }

qsizetype SignedDecFormatter::length() const { return _len; }

qsizetype SignedDecFormatter::length(quint8 byteCount) const { return digits(byteCount); }

uint16_t SignedDecFormatter::digits(quint8 byteCount) { return std::ceil(std::log10(std::pow(2, 8 * byteCount))); }

BinaryFormatter::BinaryFormatter(std::function<uint64_t()> fn, uint16_t byteCount)
    : _fn(fn), _len(byteCount), _mask(bits::mask(byteCount)) {}

QString BinaryFormatter::format() const { return u"%1"_s.arg(_mask & _fn(), length(), 2, QChar('0')); }

QString BinaryFormatter::format(quint8 byteCount) const {
  return u"%1"_s.arg(bits::mask(byteCount) & _fn(), length(byteCount), 2, QChar('0'));
}

bool BinaryFormatter::readOnly() const { return false; }

qsizetype BinaryFormatter::length() const { return 8 * _len; }

qsizetype BinaryFormatter::length(quint8 byteCount) const { return 8 * byteCount; }

MnemonicFormatter::MnemonicFormatter(std::function<QString()> fn) : _fn(fn) {}

QString MnemonicFormatter::format() const { return _fn(); }

QString MnemonicFormatter::format(quint8 byteCount) const { return _fn(); }

bool MnemonicFormatter::readOnly() const { return true; }

qsizetype MnemonicFormatter::length() const { return 7 + 2 + 3; }

qsizetype MnemonicFormatter::length(quint8 byteCount) const { return 7 + 2 + 3; }

OptionalFormatter::OptionalFormatter(QSharedPointer<RegisterFormatter> fmt, std::function<bool()> valid)
    : _fmt(fmt), _valid(valid) {}

QString OptionalFormatter::format() const { return _valid() ? _fmt->format() : ""; }

QString OptionalFormatter::format(quint8 byteCount) const { return _valid() ? _fmt->format(byteCount) : ""; }

bool OptionalFormatter::readOnly() const { return _fmt->readOnly(); }

qsizetype OptionalFormatter::length() const { return _fmt->length(); }

qsizetype OptionalFormatter::length(quint8 byteCount) const { return _fmt->length(byteCount); }

VariableByteLengthFormatter::VariableByteLengthFormatter(QSharedPointer<RegisterFormatter> fmt,
                                                         std::function<quint8()> bytes, quint8 maxBytes)
    : _fmt(fmt), _bytes(bytes), _maxBytes(maxBytes) {}

QString VariableByteLengthFormatter::format() const { return _fmt->format(_bytes()); }

QString VariableByteLengthFormatter::format(quint8 byteCount) const { return _fmt->format(byteCount); }

bool VariableByteLengthFormatter::readOnly() const { return _fmt->readOnly(); }

qsizetype VariableByteLengthFormatter::length() const { return _fmt->length(_maxBytes); }

qsizetype VariableByteLengthFormatter::length(quint8 byteCount) const { return _fmt->length(byteCount); }

ASCIIFormatter::ASCIIFormatter(std::function<uint64_t()> fn, uint16_t byteCount)
    : _fn(fn), _bytes(byteCount), _mask(bits::mask(byteCount)) {}

QString ASCIIFormatter::format() const { return format(_bytes); }

QString ASCIIFormatter::format(quint8 byteCount) const {
  auto ret = QString(byteCount, ' ');
  quint64 v = _mask & _fn();
  for (int i = 0; i < byteCount; i++) {
    ret[byteCount - i - 1] = QChar((quint8)v);
    v >>= 8;
  };
  return ret;
}

bool ASCIIFormatter::readOnly() const { return false; }

qsizetype ASCIIFormatter::length() const { return length(_bytes); }

qsizetype ASCIIFormatter::length(quint8 byteCount) const { return byteCount; }

ChoiceFormatter::ChoiceFormatter(QVector<QSharedPointer<RegisterFormatter>> formatters, qsizetype currentChoice)
    : _formatters(formatters), _currentChoice(currentChoice) {}

QString ChoiceFormatter::format() const {
  auto active = current();
  return active ? active->format() : "";
}

QString ChoiceFormatter::format(quint8 byteCount) const {
  auto active = current();
  return active ? active->format(byteCount) : "";
}

bool ChoiceFormatter::readOnly() const {
  auto active = current();
  return active ? active->readOnly() : true;
}

qsizetype ChoiceFormatter::length() const {
  auto active = current();
  return active ? active->length() : 0;
}

qsizetype ChoiceFormatter::length(quint8 byteCount) const {
  auto active = current();
  return active ? active->length(byteCount) : 0;
}

QStringList ChoiceFormatter::choices() const {
  QStringList ret;
  for (auto &f : _formatters) ret.push_back(f->describe());
  return ret;
}

qsizetype ChoiceFormatter::currentIndex() const { return _currentChoice; }

bool ChoiceFormatter::setCurrentIndex(qsizetype index) {
  if (index < _formatters.size() && index >= 0) {
    _currentChoice = index;
    return true;
  }
  return false;
}

const RegisterFormatter *ChoiceFormatter::current() const {
  if (_currentChoice < _formatters.size()) return _formatters[_currentChoice].get();
  return nullptr;
}

AutoChoice::AutoChoice(QVector<QSharedPointer<RegisterFormatter>> formatters, std::function<qsizetype()> choice)
    : _formatters(formatters), _currentChoice(choice) {}

QString AutoChoice::format() const {
  auto active = current();
  return active ? active->format() : "";
}

QString AutoChoice::format(quint8 byteCount) const {
  auto active = current();
  return active ? active->format(byteCount) : "";
}

bool AutoChoice::readOnly() const {
  auto active = current();
  return active ? active->readOnly() : true;
}

qsizetype AutoChoice::length() const {
  auto active = current();
  return active ? active->length() : 0;
}

qsizetype AutoChoice::length(quint8 byteCount) const {
  auto active = current();
  return active ? active->length(byteCount) : 0;
}

QString AutoChoice::describe() const {
  auto active = current();
  return active ? active->describe() : "";
}

const RegisterFormatter *AutoChoice::current() const {
  auto index = _currentChoice();
  if (index < _formatters.size()) return _formatters[index].get();
  return nullptr;
}
