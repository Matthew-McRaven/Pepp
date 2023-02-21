#pragma once
#include "../value.hpp"
#include <QtCore>
namespace pat::ast::argument {
class Base : public pat::ast::Value {
public:
  explicit Base();
  friend void swap(Base &first, Base &second) { using std::swap; }

  virtual bool
  isNumeric() const = 0; // Does the argument make sense as a quint64?
  virtual bool isFixedSize() const = 0; // Can different arguments of this type
                                        // occupy different numbers of bytes?
  virtual bool isWide() const = 0;      // Does the argument fit in a quint64?
  virtual bool isText() const = 0;      // Is the argument ASCII or UTF-8 text?
  virtual bool isIdentifier()
      const = 0; // Is the argument an unquoted string that is not a symbol?
  virtual bits::BitOrder endian() const override = 0;
  virtual bool value(quint8 *dest, quint16 length) const = 0;
  virtual quint64 size() const override = 0;
  virtual bool bits(QByteArray &out, bits::BitSelection src,
                    bits::BitSelection dest) const override = 0;
  virtual bool bytes(QByteArray &out, qsizetype start,
                     qsizetype length) const override = 0;
  virtual QString string() const override = 0;

protected:
  Base(const Base &other) = delete;
  Base &operator=(const Base &other) = delete;
};
} // namespace pat::ast::argument
