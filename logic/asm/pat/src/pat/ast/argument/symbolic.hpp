#pragma once
#include "./base.hpp"
namespace symbol {
class Entry;
}

namespace pat::ast::argument {
struct Symbolic : public Base {
public:
  explicit Symbolic();
  Symbolic(QSharedPointer<symbol::Entry> value, bits::BitOrder endian);
  Symbolic(const Symbolic &other);
  Symbolic(Symbolic &&other) noexcept;
  Symbolic &operator=(Symbolic other);
  friend void swap(Symbolic &first, Symbolic &second) {
    using std::swap;
    swap(first._value, second._value);
    swap(first._endian, second._endian);
  }

  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
  bool isIdentifier() const override { return false; }
  QSharedPointer<Value> clone() const override;
  virtual bits::BitOrder endian() const override;
  bool value(quint8 *dest, quint16 length) const override;
  quint64 size() const override;
  bool bits(QByteArray &out, bits::BitSelection src,
            bits::BitSelection dest) const override;
  bool bytes(QByteArray &out, qsizetype start, qsizetype length) const override;
  QString string() const override;

private:
  bits::BitOrder _endian;
  QSharedPointer<symbol::Entry> _value;
};
} // namespace pat::ast::argument
