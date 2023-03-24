#pragma once
#include "pas/bits/order.hpp"
#include <QtCore>
namespace pas::ast::value {
class Base {
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
  virtual bool isSigned() const = 0;    // If read as a number, should the value be stored in a signed typed
  virtual QSharedPointer<Base> clone() const = 0;
  virtual bool value(quint8 *dest, qsizetype length,
                     pas::bits::BitOrder destEndian =
                         pas::bits::BitOrder::BigEndian) const = 0;
  // Size and requiredBytes may mismatch if size<8 and arg is bigger than
  // 2**(8*size). e.g. size=2, arg=0x1_0000
  virtual quint64
  size() const = 0; // Number of bytes to be allocated in the bitstream.
  virtual quint64
  requiredBytes() const = 0; // Minimum number of bytes to represent value
  virtual QString string() const = 0;

protected:
  Base(const Base &other) = delete;
  Base &operator=(const Base &other) = delete;
};
} // namespace pas::ast::value
