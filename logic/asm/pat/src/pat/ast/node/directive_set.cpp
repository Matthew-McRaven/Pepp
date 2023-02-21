#include "./directive_set.hpp"

pat::ast::node::Set::Set() : Directive() {}

pat::ast::node::Set::Set(QSharedPointer<argument::Base> argument,
                         FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Directive(sourceLocation, parent), _argument(argument) {}

pat::ast::node::Set::Set(const Set &other)
    : Directive(other), _config(other._config), _argument(other._argument) {}

pat::ast::node::Set::Set(Set &&other) noexcept { swap(*this, other); }

pat::ast::node::Set &pat::ast::node::Set::operator=(Set other) {
  swap(*this, other);
  return *this;
}

const pat::ast::node::Set::Config &pat::ast::node::Set::config() const {
  return _config;
}

void pat::ast::node::Set::setConfig(Config config) { _config = config; }

QSharedPointer<pat::ast::Value> pat::ast::node::Set::clone() const {
  return QSharedPointer<Set>::create(*this);
}

pat::bits::BitOrder pat::ast::node::Set::endian() const {
  return bits::BitOrder::NotApplicable;
}

quint64 pat::ast::node::Set::size() const { return 0; }

bool pat::ast::node::Set::bits(QByteArray &out, bits::BitSelection src,
                               bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::node::Set::bytes(QByteArray &out, qsizetype start,
                                qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

QString pat::ast::node::Set::string() const {
  throw std::logic_error("Unimplemented");
}

const pat::ast::node::AddressSpan &pat::ast::node::Set::addressSpan() const {
  throw std::logic_error("Unimplemented");
}

void pat::ast::node::Set::updateAddressSpan(void *update) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::node::Set::emitsBytes() const { return false; }

void pat::ast::node::Set::setEmitsBytes(bool emitBytes) {}
