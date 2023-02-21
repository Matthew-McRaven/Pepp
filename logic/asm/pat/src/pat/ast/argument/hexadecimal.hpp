#pragma once
#include "./numeric.hpp"
namespace pat::ast::argument {
class Hexadecimal : public Numeric {
public:
  explicit Hexadecimal();
  Hexadecimal(quint64 value, quint16 size, bits::BitOrder endian);
  Hexadecimal(const Hexadecimal &other);
  Hexadecimal(Hexadecimal &&other) noexcept;
  Hexadecimal &operator=(Hexadecimal other);
  friend void swap(Hexadecimal &first, Hexadecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  QSharedPointer<Value> clone() const override;
  QString string() const override;
};
} // namespace pat::ast::argument
