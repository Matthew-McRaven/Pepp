#pragma once
#include "./base.hpp"

namespace pat::ast::argument {
class Character : public Base {
public:
  explicit Character();
  explicit Character(QString value);
  Character(const Character &other);
  Character(Character &&other) noexcept;
  Character &operator=(Character other);
  friend void swap(Character &first, Character &second) {
    using std::swap;
    swap(first._value, second._value);
    swap(first._valueAsBytes, second._valueAsBytes);
  }

  bool isNumeric() const override { return false; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
  bool isIdentifier() const override { return false; }
  QSharedPointer<Base> clone() const override;
  bool
  value(quint8 *dest, qsizetype length,
        bits::BitOrder destEndian = bits::BitOrder::BigEndian) const override;
  quint64 size() const override;
  QString string() const override;

private:
  QString _value = {};
  QByteArray _valueAsBytes = {};
};
}; // namespace pat::ast::argument
