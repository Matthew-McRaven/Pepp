#include "./directive_burn.hpp"

pat::pep::ast::node::Burn::Burn() : Directive() {}

pat::pep::ast::node::Burn::Burn(
    QSharedPointer<pat::ast::argument::Hexadecimal> argument,
    pat::ast::node::FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Directive(sourceLocation, parent), _argument(argument) {}

pat::pep::ast::node::Burn::Burn(const Burn &other)
    : Directive(other), _argument(other._argument) {}

pat::pep::ast::node::Burn::Burn(Burn &&other) noexcept { swap(*this, other); }

pat::pep::ast::node::Burn &pat::pep::ast::node::Burn::operator=(Burn other) {
  swap(*this, other);
  return *this;
}

pat::pep::ast::node::Burn::ValidateResult
pat::pep::ast::node::Burn::validate_argument(
    QSharedPointer<const pat::ast::argument::Hexadecimal> argument) {
  return {.valid = true};
}

QSharedPointer<pat::ast::Value> pat::pep::ast::node::Burn::clone() const {
  return QSharedPointer<Burn>::create(*this);
}

pat::bits::BitOrder pat::pep::ast::node::Burn::endian() const {
  return bits::BitOrder::NotApplicable;
}

quint64 pat::pep::ast::node::Burn::size() const { return 0; }

bool pat::pep::ast::node::Burn::bits(QByteArray &out, bits::BitSelection src,
                                     bits::BitSelection dest) const {
  return true;
}

bool pat::pep::ast::node::Burn::bytes(QByteArray &out, qsizetype start,
                                      qsizetype length) const {
  return true;
}

QString pat::pep::ast::node::Burn::string() const {
  throw std::logic_error("Unimplemented");
}

const pat::ast::node::AddressSpan &
pat::pep::ast::node::Burn::addressSpan() const {
  throw std::logic_error("Unimplemented");
}

void pat::pep::ast::node::Burn::updateAddressSpan(void *update) const {
  throw std::logic_error("Unimplemented");
}

bool pat::pep::ast::node::Burn::emitsBytes() const { return false; }

void pat::pep::ast::node::Burn::setEmitsBytes(bool emitBytes) {}
