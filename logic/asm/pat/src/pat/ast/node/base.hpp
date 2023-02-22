#pragma once
#include "pat/bits/order.hpp"
namespace pat::ast::node {
struct FileLocation {};
struct AddressSpan {};
struct Base {
  explicit Base();
  Base(FileLocation sourceLocation, QWeakPointer<Base> parent = {});
  friend void swap(Base &first, Base &second) {
    using std::swap;
    swap(first._parent, second._parent);
    swap(first._sourceLocation, second._sourceLocation);
    swap(first._listingLocation, second._listingLocation);
  }

  // ast::Value interface
  // Nodes with children must manually update children's parent to this.
  virtual QSharedPointer<Base> clone() const = 0;
  virtual quint64 size() const = 0;
  virtual bool
  value(quint8 *dest, qsizetype length,
        bits::BitOrder destEndian = bits::BitOrder::BigEndian) const = 0;
  virtual QString string() const = 0;

  // ast::node::Base interface
  virtual const AddressSpan &addressSpan() const = 0;
  virtual void updateAddressSpan(void *update) const = 0;
  virtual bool emitsBytes() const = 0;
  virtual void setEmitsBytes(bool emitBytes) = 0;

  // Helpers implemented locally
  QWeakPointer<Base> parent();
  QWeakPointer<const Base> parent() const;
  void setParent(QWeakPointer<Base> parent);
  FileLocation sourceLocation() const;
  std::optional<FileLocation> listingLocation() const;
  void setListingLocation(FileLocation listingLocation);

protected:
  Base(const Base &other);
  Base &operator=(const Base &other);
  QWeakPointer<Base> _parent = {};
  FileLocation _sourceLocation = {};
  std::optional<FileLocation> _listingLocation = std::nullopt;
};

} // namespace pat::ast::node
