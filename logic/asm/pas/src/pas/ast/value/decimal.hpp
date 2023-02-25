#pragma once
#include "./numeric.hpp"

namespace pas::ast::value {
class SignedDecimal : public Numeric {
public:
  explicit SignedDecimal();
  SignedDecimal(qint64 value, quint16 size, bits::BitOrder endian);
  SignedDecimal(const SignedDecimal &other);
  SignedDecimal(SignedDecimal &&other) noexcept;
  SignedDecimal &operator=(SignedDecimal other);
  friend void swap(SignedDecimal &first, SignedDecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  QSharedPointer<Base> clone() const override;
  QString string() const override;
};

class UnsignedDecimal : public Numeric {
public:
  explicit UnsignedDecimal();
  UnsignedDecimal(quint64 value, quint16 size, bits::BitOrder endian);
  UnsignedDecimal(const UnsignedDecimal &other);
  UnsignedDecimal(UnsignedDecimal &&other) noexcept;
  UnsignedDecimal &operator=(UnsignedDecimal other);
  friend void swap(UnsignedDecimal &first, UnsignedDecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  QSharedPointer<Base> clone() const override;
  QString string() const override;
};
} // namespace pas::ast::value
