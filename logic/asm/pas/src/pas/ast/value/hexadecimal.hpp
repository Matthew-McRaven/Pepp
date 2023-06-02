#pragma once
#include "./numeric.hpp"
#include "pas/pas_globals.hpp"

namespace pas::ast::value {
class PAS_EXPORT Hexadecimal : public Numeric {
public:
  explicit Hexadecimal();
  Hexadecimal(quint64 value, quint16 size);
  Hexadecimal(const Hexadecimal &other);
  Hexadecimal(Hexadecimal &&other) noexcept;
  Hexadecimal &operator=(Hexadecimal other);
  friend void swap(Hexadecimal &first, Hexadecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  QSharedPointer<Base> clone() const override;
  QString string() const override;
  QString rawString() const override;
};
} // namespace pas::ast::value
