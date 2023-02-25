#pragma once
#include "./base.hpp"

namespace pas::ast::value {
struct Numeric : public Base {
public:
  explicit Numeric();
  Numeric(qint64 value, quint8 size, bits::BitOrder endian);
  friend void swap(Numeric &first, Numeric &second) {
    using std::swap;
    swap((Base &)first, (Base &)second);
    swap(first._size, second._size);
    swap(first._value, second._value);
  }

  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
  bool isIdentifier() const override { return false; }
  virtual QSharedPointer<Base> clone() const override = 0;
  bool
  value(quint8 *dest, qsizetype length,
        bits::BitOrder targetEndian = bits::BitOrder::BigEndian) const override;
  quint64 size() const override;
  virtual QString string() const override = 0;

protected:
  Numeric(const Numeric &other);
  Numeric &operator=(const Numeric &other);
  quint8 _size = 0;
  qint64 _value = 0;
};
} // namespace pas::ast::value
