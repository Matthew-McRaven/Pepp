#include "character.hpp"
#include "../../bits/operations.hpp"
pat::ast::argument::Character::Character(QString value)
    : _value(value), _valueAsBytes({}) {}

QSharedPointer<pat::ast::Value> pat::ast::argument::Character::clone() const {
  return QSharedPointer<Character>::create(_value);
}

pat::bits::BitOrder pat::ast::argument::Character::endian() const {
  return bits::BitOrder::NotApplicable;
}

bool pat::ast::argument::Character::value(quint8 *dest, quint16 length) const {
  return bits::copy(reinterpret_cast<const quint8 *>(_valueAsBytes.data()),
                    bits::hostOrder(), size(), dest, bits::hostOrder(), length);
}

quint64 pat::ast::argument::Character::size() const { return _value.size(); }

bool pat::ast::argument::Character::bits(QByteArray &out,
                                         bits::BitSelection src,
                                         bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::argument::Character::bytes(QByteArray &out, qsizetype start,
                                          qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

QString pat::ast::argument::Character::string() const {
  throw std::logic_error("Unimplemented");
}
