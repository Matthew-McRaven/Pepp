#pragma once
#include "./base.hpp"
#include "pas/pas_globals.hpp"

namespace pas::ast::value {
struct PAS_EXPORT ShortString : public Base {
public:
  explicit ShortString();
  ShortString(QString value, quint8 size, bits::Order endian);
  ShortString(const ShortString &other);
  ShortString(ShortString &&other) noexcept;
  ShortString &operator=(ShortString other);
  friend void swap(ShortString &first, ShortString &second) {
    using std::swap;
    swap(first._size, second._size);
    swap(first._value, second._value);
    swap(first._valueAsBytes, second._valueAsBytes);
  }

  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return true; }
  bool isIdentifier() const override { return false; }
  bool isSigned() const override { return false; }
  QSharedPointer<Base> clone() const override;
  void value(bits::span<quint8> dest,
             bits::Order destEndian = bits::hostOrder()) const override;
  quint64 size() const override;
  quint64 requiredBytes() const override;
  QString string() const override;
  QString rawString() const override;

private:
  quint8 _size = 0;
  QString _value = {};
  QByteArray _valueAsBytes = {};
};

struct PAS_EXPORT LongString : public Base {
public:
  explicit LongString();
  LongString(QString value, bits::Order endian);
  LongString(const LongString &other);
  LongString(LongString &&other) noexcept;
  LongString &operator=(LongString other);
  friend void swap(LongString &first, LongString &second) {
    using std::swap;
    swap(first._value, second._value);
    swap(first._valueAsBytes, second._valueAsBytes);
  }

  bool isNumeric() const override { return false; }
  bool isFixedSize() const override { return false; }
  bool isWide() const override { return size() > 8; }
  bool isText() const override { return true; }
  bool isIdentifier() const override { return false; }
  bool isSigned() const override { return false; }
  QSharedPointer<Base> clone() const override;
  void value(bits::span<quint8> dest,
             bits::Order destEndian = bits::hostOrder()) const override;
  quint64 size() const override;
  quint64 requiredBytes() const override;
  QString string() const override;
  QString rawString() const override;

private:
  QString _value = {};
  QByteArray _valueAsBytes = {};
};
} // namespace pas::ast::value
