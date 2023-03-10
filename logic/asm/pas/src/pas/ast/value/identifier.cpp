#include "./identifier.hpp"

pas::ast::value::Identifier::Identifier() : Base() {}

pas::ast::value::Identifier::Identifier(QString value) : _value(value) {}

pas::ast::value::Identifier::Identifier(const Identifier &other)
    : Base(), _value(other._value) {}

pas::ast::value::Identifier::Identifier(Identifier &&other) noexcept {
  swap(*this, other);
}

pas::ast::value::Identifier &
pas::ast::value::Identifier::operator=(Identifier other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base>
pas::ast::value::Identifier::clone() const {
  return QSharedPointer<Identifier>::create(*this);
}

bool pas::ast::value::Identifier::value(quint8 *dest, qsizetype length,
                                        bits::BitOrder destEndian) const {
  return true;
}

quint64 pas::ast::value::Identifier::size() const { return 0; }

quint64 pas::ast::value::Identifier::requiredBytes() const { return 0; }

QString pas::ast::value::Identifier::string() const { return _value; }
