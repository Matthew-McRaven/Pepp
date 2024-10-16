#include "formats.hpp"
#include "bits/mask.hpp"
using namespace Qt::StringLiterals;

TextFormatter::TextFormatter(QString value) : _value(value) {}

QString TextFormatter::format() const { return _value; }

QString TextFormatter::format(quint8 byteCount) const { return _value; }

bool TextFormatter::readOnly() const { return true; }

qsizetype TextFormatter::length() const { return _value.length(); }

qsizetype TextFormatter::length(quint8 byteCount) const { return _value.length(); }

HexFormatter::HexFormatter(std::function<uint64_t()> fn, uint16_t byteCount)
    : _fn(fn), _bytes(byteCount), _mask(bits::mask(byteCount)) {}

QString HexFormatter::format() const { return u"0x%1"_s.arg(_mask & _fn(), _bytes * 2, 16, QChar('0')); }

QString HexFormatter::format(quint8 byteCount) const {
  return u"0x%1"_s.arg(bits::mask(byteCount) & _fn(), byteCount * 2, 16, QChar('0'));
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
  if (auto v = _fn(); v < 0)
    // Limit V to the range expressable with an N-bit unsigned int before restoring the sign.
    return QString::number(~(_mask & ~v));
  return QString::number(_mask & _fn());
}

QString SignedDecFormatter::format(quint8 byteCount) const {
  if (auto v = _fn(); v < 0)
    // Limit V to the range expressable with an N-bit unsigned int before restoring the sign.
    return QString::number(~(bits::mask(byteCount) & ~v));
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
