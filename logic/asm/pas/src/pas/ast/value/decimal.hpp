#pragma once
#include "./numeric.hpp"

namespace pas::ast::value {
class SignedDecimal : public Numeric {
public:
  explicit SignedDecimal();
  SignedDecimal(qint64 value, quint16 size);
  SignedDecimal(const SignedDecimal &other);
  SignedDecimal(SignedDecimal &&other) noexcept;
  SignedDecimal &operator=(SignedDecimal other);
  friend void swap(SignedDecimal &first, SignedDecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }
  bool isSigned() const override { return true; }
  QSharedPointer<Base> clone() const override;
  // Must negate value before computing max bit value
  quint64 requiredBytes() const override;
  QString string() const override;
  QString rawString() const override;
};

class UnsignedDecimal : public Numeric {
public:
  explicit UnsignedDecimal();
  UnsignedDecimal(quint64 value, quint16 size);
  UnsignedDecimal(const UnsignedDecimal &other);
  UnsignedDecimal(UnsignedDecimal &&other) noexcept;
  UnsignedDecimal &operator=(UnsignedDecimal other);
  friend void swap(UnsignedDecimal &first, UnsignedDecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  QSharedPointer<Base> clone() const override;
  QString string() const override;
  QString rawString() const override;
};
} // namespace pas::ast::value
