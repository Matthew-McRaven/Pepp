#pragma once
#include "./base.hpp"

namespace pas::ast::value {
struct Identifier : public Base {
public:
  explicit Identifier();
  explicit Identifier(QString value);
  Identifier(const Identifier &other);
  Identifier(Identifier &&other) noexcept;
  Identifier &operator=(Identifier other);
  friend void swap(Identifier &first, Identifier &second) {
    using std::swap;
    swap(first._value, second._value);
  }

  bool isNumeric() const override { return false; }
  bool isFixedSize() const override { return false; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
  bool isIdentifier() const override { return true; }
  bool isSigned() const override { return false; }
  QSharedPointer<Base> clone() const override;
  void value(quint8 *dest, qsizetype length,
             bits::Order destEndian = bits::hostOrder()) const override;
  quint64 size() const override;
  quint64 requiredBytes() const override;
  QString string() const override;
  QString rawString() const override;

private:
  QString _value = {};
};
} // namespace pas::ast::value
