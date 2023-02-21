#pragma once
#include "./base.hpp"

namespace pat::ast::argument {
struct Numeric : public Base {
public:
  explicit Numeric();
  Numeric(qint64 value, quint8 size, bits::BitOrder endian);
  friend void swap(Numeric &first, Numeric &second) {
    using std::swap;
    swap((Base &)first, (Base &)second);
    swap(first._endian, second._endian);
    swap(first._size, second._size);
    swap(first._value, second._value);
  }

  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
  bool isIdentifier() const override { return false; }
  virtual QSharedPointer<Value> clone() const override = 0;
  bits::BitOrder endian() const override;
  bool value(quint8 *dest, quint16 length) const override;
  quint64 size() const override;
  bool bits(QByteArray &out, bits::BitSelection src,
            bits::BitSelection dest) const override;
  bool bytes(QByteArray &out, qsizetype start, qsizetype length) const override;
  virtual QString string() const override = 0;

protected:
  Numeric(const Numeric &other);
  Numeric &operator=(const Numeric &other);
  bits::BitOrder _endian = bits::BitOrder::NotApplicable;
  quint8 _size = 0;
  qint64 _value = 0;
};
} // namespace pat::ast::argument
