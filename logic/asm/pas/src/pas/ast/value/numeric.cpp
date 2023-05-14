#include "numeric.hpp"
#include "bits/operations/copy.hpp"
pas::ast::value::Numeric::Numeric() : Base() {}

pas::ast::value::Numeric::Numeric(qint64 value, quint8 size)
    : _size(size), _value(value) {
  if (size > 8)
    throw std::logic_error("Numeric constants must be <=8 bytes");
}

void pas::ast::value::Numeric::value(bits::span<quint8> dest,
                                     bits::Order destEndian) const {
  bits::memcpy_endian(dest, destEndian,
                      bits::span<const quint8>{
                          reinterpret_cast<const quint8 *>(&_value), size()},
                      bits::hostOrder());
}

quint64 pas::ast::value::Numeric::size() const { return _size; }

quint64 pas::ast::value::Numeric::requiredBytes() const {
  return ceil(log2(_value + 1) / 8);
}

pas::ast::value::Numeric::Numeric(const Numeric &other)
    : Base(), _size(other._size), _value(other._value) {}

pas::ast::value::Numeric &
pas::ast::value::Numeric::operator=(const Numeric &other) {
  // Base::operator=(other); // Needed if we add data to Base.
  this->_size = other._size;
  this->_value = other._value;
  return *this;
}
