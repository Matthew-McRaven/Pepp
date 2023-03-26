#include "hexadecimal.hpp"

pas::ast::value::Hexadecimal::Hexadecimal() : Numeric() {}

pas::ast::value::Hexadecimal::Hexadecimal(quint64 value, quint16 size)
    : Numeric(value, size) {}

pas::ast::value::Hexadecimal::Hexadecimal(const Hexadecimal &other)
    : Numeric(other) {}

pas::ast::value::Hexadecimal::Hexadecimal(Hexadecimal &&other) noexcept {
  swap(*this, other);
}

pas::ast::value::Hexadecimal &
pas::ast::value::Hexadecimal::operator=(Hexadecimal other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base>
pas::ast::value::Hexadecimal::clone() const {
  return QSharedPointer<Hexadecimal>::create(*this);
}

QString pas::ast::value::Hexadecimal::string() const {
    return u"0x%1"_qs.arg(QString::number(_value, 16).toUpper(), 2*_size, QChar('0'));
}
