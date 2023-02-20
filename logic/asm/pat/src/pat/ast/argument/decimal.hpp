#pragma once
#include "./numeric.hpp"

namespace pat::ast::argument {
class SignedDecimal : public Numeric {
public:
  SignedDecimal(qint64 value, quint16 size, bits::BitOrder endian);
  QSharedPointer<Value> clone() const override;
  QString string() const override;
};

class UnsignedDecimal : public Numeric {
public:
  UnsignedDecimal(quint64 value, quint16 size, bits::BitOrder endian);
  QSharedPointer<Value> clone() const override;
  QString string() const override;
};
} // namespace pat::ast::argument
