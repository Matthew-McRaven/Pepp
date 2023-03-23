#include "./decimal.hpp"

pas::ast::value::SignedDecimal::SignedDecimal() : Numeric() {}

pas::ast::value::SignedDecimal::SignedDecimal(qint64 value, quint16 size)
    : Numeric(value, size) {}

pas::ast::value::SignedDecimal::SignedDecimal(const SignedDecimal &other)
    : Numeric(other) {}

pas::ast::value::SignedDecimal::SignedDecimal(SignedDecimal &&other) noexcept {
  swap(*this, other);
}

pas::ast::value::SignedDecimal &
pas::ast::value::SignedDecimal::operator=(SignedDecimal other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base>
pas::ast::value::SignedDecimal::clone() const {
  return QSharedPointer<SignedDecimal>::create(*this);
}

quint64 pas::ast::value::SignedDecimal::requiredBytes() const {
    // Handle _value = 0b1000...0, otherwise we take log of negative number.
    if(_value*-1==_value) return sizeof(_value);
    // Must subtract 1 bit (log2(n)+1), because the top order bit holds sign, not data.
    return ceil((log2(-1*_value)+1) / 8);
}

QString pas::ast::value::SignedDecimal::string() const {
  return QString::number(_value);
}

pas::ast::value::UnsignedDecimal::UnsignedDecimal() : Numeric() {}

pas::ast::value::UnsignedDecimal::UnsignedDecimal(quint64 value, quint16 size)
    : Numeric(value, size) {}

pas::ast::value::UnsignedDecimal::UnsignedDecimal(const UnsignedDecimal &other)
    : Numeric(other) {}

pas::ast::value::UnsignedDecimal::UnsignedDecimal(
    UnsignedDecimal &&other) noexcept {
  swap(*this, other);
}

pas::ast::value::UnsignedDecimal &
pas::ast::value::UnsignedDecimal::operator=(UnsignedDecimal other) {
  swap(*this, other);
  return *this;
}



QSharedPointer<pas::ast::value::Base>
pas::ast::value::UnsignedDecimal::clone() const {
  return QSharedPointer<UnsignedDecimal>::create(*this);
}


QString pas::ast::value::UnsignedDecimal::string() const {
  return QString::number(_value);
}
