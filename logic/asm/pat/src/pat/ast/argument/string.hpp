#pragma once
#include "./base.hpp"

namespace pat::ast::argument {
struct ShortString : public Base {
public:
  ShortString(QString value, quint8 size, bits::BitOrder endian);
  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return true; }
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
  const bits::BitOrder _endian;
  const quint8 _size;
  const QString _value;
  QByteArray _valueAsBytes;
};

struct LongString : public Base {
public:
  LongString(QString value, bits::BitOrder endian);
  bool isNumeric() const override { return false; }
  bool isFixedSize() const override { return false; }
  bool isWide() const override { return size() > 8; }
  bool isText() const override { return true; }
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
  const bits::BitOrder _endian;
  const QString _value;
  QByteArray _valueAsBytes;
};
} // namespace pat::ast::argument
