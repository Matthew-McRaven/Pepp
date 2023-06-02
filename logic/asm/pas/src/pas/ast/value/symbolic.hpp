#pragma once
#include "./base.hpp"
#include "pas/pas_globals.hpp"

namespace symbol {
class Entry;
}

namespace pas::ast::value {
struct PAS_EXPORT Symbolic : public Base {
public:
  explicit Symbolic();
  Symbolic(QSharedPointer<symbol::Entry> value);
  Symbolic(const Symbolic &other);
  Symbolic(Symbolic &&other) noexcept;
  Symbolic &operator=(Symbolic other);
  friend void swap(Symbolic &first, Symbolic &second) {
    using std::swap;
    swap(first._value, second._value);
  }
  QSharedPointer<symbol::Entry> symbol();
  QSharedPointer<const symbol::Entry> symbol() const;
  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
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
  QSharedPointer<symbol::Entry> _value = nullptr;
};
} // namespace pas::ast::value
