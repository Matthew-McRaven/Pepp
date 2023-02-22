#include "./directive_end.hpp"

pat::pep::ast::node::End::End() : Directive() {}

pat::pep::ast::node::End::End(bool doNotEmit,
                              pat::ast::node::FileLocation sourceLocation,
                              QWeakPointer<Base> parent)
    : Directive(sourceLocation, parent), _doNotEmit(doNotEmit) {}

pat::pep::ast::node::End::End(const End &other)
    : Directive(other), _doNotEmit(other._doNotEmit) {}

pat::pep::ast::node::End::End(End &&other) noexcept { swap(*this, other); }

pat::pep::ast::node::End &pat::pep::ast::node::End::operator=(End other) {
  swap(*this, other);
  return *this;
}

bool pat::pep::ast::node::End::doNotEmit() const { return _doNotEmit; }

void pat::pep::ast::node::End::setDoNotEmit(bool doNotEmit) {
  _doNotEmit = doNotEmit;
}

QSharedPointer<pat::ast::node::Base> pat::pep::ast::node::End::clone() const {
  return QSharedPointer<End>::create(*this);
}

quint64 pat::pep::ast::node::End::size() const { return 0; }

bool pat::pep::ast::node::End::value(quint8 *dest, qsizetype length,
                                     bits::BitOrder destEndian) const {
  return true;
}

QString pat::pep::ast::node::End::string() const {
  throw std::logic_error("Unimplemented");
}

const pat::ast::node::AddressSpan &
pat::pep::ast::node::End::addressSpan() const {
  throw std::logic_error("Unimplemented");
}

void pat::pep::ast::node::End::updateAddressSpan(void *update) const {
  throw std::logic_error("Unimplemented");
}

bool pat::pep::ast::node::End::emitsBytes() const { return false; }

void pat::pep::ast::node::End::setEmitsBytes(bool emitBytes) {}
