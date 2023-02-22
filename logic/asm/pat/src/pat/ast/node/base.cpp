#include "./base.hpp"
pat::ast::node::Base::Base() {}

pat::ast::node::Base::Base(FileLocation sourceLocation,
                           QWeakPointer<Base> parent)
    : _parent(parent), _sourceLocation(sourceLocation) {}

QWeakPointer<pat::ast::node::Base> pat::ast::node::Base::parent() {
  return _parent;
}

QWeakPointer<const pat::ast::node::Base> pat::ast::node::Base::parent() const {
  return _parent;
}

void pat::ast::node::Base::setParent(QWeakPointer<Base> parent) {
  _parent = parent;
}

pat::ast::node::FileLocation pat::ast::node::Base::sourceLocation() const {
  return _sourceLocation;
}

std::optional<pat::ast::node::FileLocation>
pat::ast::node::Base::listingLocation() const {
  return _listingLocation;
}

void pat::ast::node::Base::setListingLocation(FileLocation listingLocation) {}

pat::ast::node::Base::Base(const Base &other)
    : _parent(other._parent), _sourceLocation(other._sourceLocation),
      _listingLocation(other._listingLocation) {}

pat::ast::node::Base &pat::ast::node::Base::operator=(const Base &other) {
  Base::operator=(other);
  this->_parent = other._parent;
  this->_sourceLocation = other._sourceLocation;
  this->_listingLocation = other._listingLocation;
  return *this;
}
