#include "directive_symbolic.hpp"

pat::pep::ast::node::Symbolic::Symbolic() : Directive() {}

pat::pep::ast::node::Symbolic::Symbolic(
    QSharedPointer<pat::ast::argument::Symbolic> argument,
    pat::ast::node::FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Directive(sourceLocation, parent), _argument(argument) {}

pat::bits::BitOrder pat::pep::ast::node::Symbolic::endian() const {
  return bits::BitOrder::NotApplicable;
}

quint64 pat::pep::ast::node::Symbolic::size() const { return 0; }

bool pat::pep::ast::node::Symbolic::bits(QByteArray &out,
                                         bits::BitSelection src,
                                         bits::BitSelection dest) const {
  return true;
}

bool pat::pep::ast::node::Symbolic::bytes(QByteArray &out, qsizetype start,
                                          qsizetype length) const {
  return true;
}

const pat::ast::node::AddressSpan &
pat::pep::ast::node::Symbolic::addressSpan() const {
  throw std::logic_error("Unimplemented");
}

void pat::pep::ast::node::Symbolic::updateAddressSpan(void *update) const {
  throw std::logic_error("Unimplemented");
}

bool pat::pep::ast::node::Symbolic::emitsBytes() const { return false; }

void pat::pep::ast::node::Symbolic::setEmitsBytes(bool emitBytes) {}

pat::pep::ast::node::Symbolic::Symbolic(const Symbolic &other)
    : Directive(other), _argument(other._argument) {}

pat::pep::ast::node::Symbolic &
pat::pep::ast::node::Symbolic::operator=(Symbolic &other) {
  Directive::operator=(other);
  this->_argument = other._argument;
  return *this;
}
