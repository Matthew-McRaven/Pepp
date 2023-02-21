#include "./directive_string.hpp"
#include "../argument/base.hpp"
pat::ast::node::ASCII::ASCII() : Directive() {}

pat::ast::node::ASCII::ASCII(QSharedPointer<argument::Base> argument,
                             FileLocation sourceLocation,
                             QWeakPointer<Base> parent)
    : Directive(sourceLocation, parent), _argument(argument) {}

pat::ast::node::ASCII::ASCII(const ASCII &other)
    : Directive(other), _config(other._config), _argument(other._argument),
      _emitsBytes(other._emitsBytes) {}

pat::ast::node::ASCII::ASCII(ASCII &&other) noexcept { swap(*this, other); }

pat::ast::node::ASCII &pat::ast::node::ASCII::operator=(ASCII other) {
  swap(*this, other);
  return *this;
}

const pat::ast::node::ASCII::Config &pat::ast::node::ASCII::config() const {
  return _config;
}

void pat::ast::node::ASCII::setConfig(Config config) { _config = config; }

pat::ast::node::ASCII::ValidateResult pat::ast::node::ASCII::validate_argument(
    QSharedPointer<const argument::Base> argument) {
  if (argument->isText())
    return {.valid = true};
  return {.valid = false, .errorMessage = u"Argument must be string"_qs};
}

QSharedPointer<pat::ast::Value> pat::ast::node::ASCII::clone() const {
  return QSharedPointer<ASCII>::create(*this);
}

pat::bits::BitOrder pat::ast::node::ASCII::endian() const {
  return _argument->endian();
}

quint64 pat::ast::node::ASCII::size() const { return _argument->size(); }

bool pat::ast::node::ASCII::bits(QByteArray &out, bits::BitSelection src,
                                 bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::node::ASCII::bytes(QByteArray &out, qsizetype start,
                                  qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

QString pat::ast::node::ASCII::string() const {
  throw std::logic_error("Unimplemented");
}

const pat::ast::node::AddressSpan &pat::ast::node::ASCII::addressSpan() const {
  throw std::logic_error("Unimplemented");
}

void pat::ast::node::ASCII::updateAddressSpan(void *update) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::node::ASCII::emitsBytes() const { return _emitsBytes; }

void pat::ast::node::ASCII::setEmitsBytes(bool emitBytes) {
  _emitsBytes = emitBytes;
}
