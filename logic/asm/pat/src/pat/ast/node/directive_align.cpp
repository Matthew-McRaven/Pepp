#include "./directive_align.hpp"
#include "../argument/base.hpp"
pat::ast::node::Align::Align() : Directive() {}

pat::ast::node::Align::Align(QSharedPointer<argument::Base> argument,
                             FileLocation sourceLocation,
                             QWeakPointer<Base> parent)
    : Directive(sourceLocation, parent), _argument(argument) {}

pat::ast::node::Align::Align(const Align &other)
    : Directive(other), _config(other._config), _argument(other._argument),
      _pad(other._pad), _emitsBytes(other._emitsBytes) {}

pat::ast::node::Align::Align(Align &&other) noexcept { swap(*this, other); }

pat::ast::node::Align &pat::ast::node::Align::operator=(Align other) {
  swap(*this, other);
  return *this;
}

const pat::ast::node::Align::Config &pat::ast::node::Align::config() const {
  return _config;
}

void pat::ast::node::Align::setConfig(Config config) { _config = config; }

void pat::ast::node::Align::setPad(QSharedPointer<argument::Base> pad) {
  _pad = pad;
}

QSharedPointer<pat::ast::Value> pat::ast::node::Align::clone() const {
  return QSharedPointer<Align>::create(*this);
}

pat::bits::BitOrder pat::ast::node::Align::endian() const {
  return _argument->endian();
}

quint64 pat::ast::node::Align::size() const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::node::Align::bits(QByteArray &out, bits::BitSelection src,
                                 bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::node::Align::bytes(QByteArray &out, qsizetype start,
                                  qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

QString pat::ast::node::Align::string() const {
  throw std::logic_error("Unimplemented");
}

const pat::ast::node::AddressSpan &pat::ast::node::Align::addressSpan() const {
  throw std::logic_error("Unimplemented");
}

void pat::ast::node::Align::updateAddressSpan(void *update) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::node::Align::emitsBytes() const { return _emitsBytes; }

void pat::ast::node::Align::setEmitsBytes(bool emitBytes) {
  _emitsBytes = emitBytes;
}
