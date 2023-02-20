#pragma once
#include "./numeric.hpp"
namespace pat::ast::argument {
class Hexadecimal : public Numeric {
public:
  Hexadecimal(quint64 value, quint16 size, bits::BitOrder endian);
  QSharedPointer<Value> clone() const override;
  QString string() const override;
};
} // namespace pat::ast::argument
