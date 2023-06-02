#pragma once
#include "bits/order.hpp"
#include "bits/span.hpp"
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast::value {
class PAS_EXPORT Base {
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
  virtual bool isSigned() const = 0; // If read as a number, should the value be
                                     // stored in a signed typed
  virtual QSharedPointer<Base> clone() const = 0;
  virtual void value(bits::span<quint8> dest,
                     bits::Order destEndian = bits::Order::BigEndian) const = 0;
  // Size and requiredBytes may mismatch if size<8 and arg is bigger than
  // 2**(8*size). e.g. size=2, arg=0x1_0000
  virtual quint64
  size() const = 0; // Number of bytes to be allocated in the bitstream.
  virtual quint64
  requiredBytes() const = 0; // Minimum number of bytes to represent value
  virtual QString string() const = 0;
  virtual QString
  rawString() const = 0; // like string(), except without quotation marks.

protected:
  Base(const Base &other) = delete;
  Base &operator=(const Base &other) = delete;
};
} // namespace pas::ast::value
