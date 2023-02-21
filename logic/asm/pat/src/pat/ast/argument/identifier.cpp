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

QSharedPointer<pat::ast::Value> pat::ast::argument::Identifier::clone() const {
  return QSharedPointer<Identifier>::create(*this);
}

pat::bits::BitOrder pat::ast::argument::Identifier::endian() const {
  return bits::BitOrder::NotApplicable;
}

bool pat::ast::argument::Identifier::value(quint8 *dest, quint16 length) const {
  return true;
}

quint64 pat::ast::argument::Identifier::size() const { return 0; }

bool pat::ast::argument::Identifier::bits(QByteArray &out,
                                          bits::BitSelection src,
                                          bits::BitSelection dest) const {
  return true;
}

bool pat::ast::argument::Identifier::bytes(QByteArray &out, qsizetype start,
                                           qsizetype length) const {
  return true;
}

QString pat::ast::argument::Identifier::string() const { return _value; }
