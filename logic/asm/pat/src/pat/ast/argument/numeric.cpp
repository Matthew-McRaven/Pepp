#include "numeric.hpp"
#include "../../bits/operations.hpp"
pat::ast::argument::Numeric::Numeric() : Base() {}

pat::ast::argument::Numeric::Numeric(qint64 value, quint8 size,
                                     bits::BitOrder endian)
    : _size(size), _value(value) {
  if (size > 8)
    throw std::logic_error("Numeric constants must be <=8 bytes");
}

bool pat::ast::argument::Numeric::value(quint8 *dest, qsizetype length,
                                        bits::BitOrder destEndian) const {
  return bits::copy(reinterpret_cast<const quint8 *>(&_value),
                    bits::hostOrder(), _size, dest, destEndian, length);
}

quint64 pat::ast::argument::Numeric::size() const { return _size; }

pat::ast::argument::Numeric::Numeric(const Numeric &other)
    : Base(), _size(other._size), _value(other._value) {}

pat::ast::argument::Numeric &
pat::ast::argument::Numeric::operator=(const Numeric &other) {
  // Base::operator=(other); // Needed if we add data to Base.
  this->_size = other._size;
  this->_value = other._value;
  return *this;
}
