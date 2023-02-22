#include "./identifier.hpp"

pat::ast::argument::Identifier::Identifier() : Base() {}

pat::ast::argument::Identifier::Identifier(QString value) : _value(value) {}

pat::ast::argument::Identifier::Identifier(const Identifier &other)
    : Base(), _value(other._value) {}

pat::ast::argument::Identifier::Identifier(Identifier &&other) noexcept {
  swap(*this, other);
}

pat::ast::argument::Identifier &
pat::ast::argument::Identifier::operator=(Identifier other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::argument::Base>
pat::ast::argument::Identifier::clone() const {
  return QSharedPointer<Identifier>::create(*this);
}

bool pat::ast::argument::Identifier::value(quint8 *dest, qsizetype length,
                                           bits::BitOrder destEndian) const {
  return true;
}

quint64 pat::ast::argument::Identifier::size() const { return 0; }

QString pat::ast::argument::Identifier::string() const { return _value; }
