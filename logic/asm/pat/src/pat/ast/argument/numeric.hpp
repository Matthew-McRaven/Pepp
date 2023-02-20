#pragma once
#include "./base.hpp"

namespace pat::ast::argument {
struct Numeric : public Base {
public:
  Numeric(qint64 value, quint8 size, bits::BitOrder endian);
  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
  bool isIdentifier() const override { return false; }
  virtual QSharedPointer<Value> clone() const override = 0;
  bits::BitOrder endian() const override;
  bool value(quint8 *dest, quint16 length) const override;
  quint64 size() const override;
  bool bits(QByteArray &out, bits::BitSelection src,
            bits::BitSelection dest) const override;
  bool bytes(QByteArray &out, qsizetype start, qsizetype length) const override;
  virtual QString string() const override = 0;

protected:
  const bits::BitOrder _endian;
  const quint8 _size;
  const qint64 _value;
};
} // namespace pat::ast::argument
