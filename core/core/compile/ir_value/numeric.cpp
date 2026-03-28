#include "numeric.hpp"
#include <fmt/format.h>
#include <iostream>
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/log2.hpp"

pepp::ast::Numeric::Numeric() noexcept : IRValue() {}

pepp::ast::Numeric::Numeric(u64 value, u8 size) noexcept : IRValue(), _size(size), _value(value) {
  if (size > 8) {
    static const char *const e = "Numeric constants must be <=8 bytes";
    std::cerr << e;
    throw std::logic_error(e);
  }
}

u64 pepp::ast::Numeric::minimum_size() const noexcept { return ceil(log2(_value + 1) / 8); }

[[nodiscard]]
u32 pepp::ast::Numeric::serialize(bits::span<u8> dest, bits::Order destEndian, u32 max_size) const noexcept {
  using size_type = bits::span<const u8>::size_type;
  auto size = std::min<size_type>(max_size, serialized_size());
  std::span<const u8> src(reinterpret_cast<const u8 *>(&_value), sizeof(_value));
  bits::memcpy_endian(dest, destEndian, src, bits::hostOrder());
  return size;
}

pepp::ast::Numeric::Numeric(const Numeric &other) : IRValue(), _size(other._size), _value(other._value) {}

pepp::ast::Numeric &pepp::ast::Numeric::operator=(const Numeric &other) {
  // Base::operator=(other); // Needed if we add data to Base.
  this->_size = other._size;
  this->_value = other._value;
  return *this;
}

pepp::ast::SignedDecimal::SignedDecimal() noexcept : Numeric() {}

pepp::ast::SignedDecimal::SignedDecimal(i64 value, u8 size) noexcept : Numeric(value, size) {}

pepp::ast::SignedDecimal::SignedDecimal(const SignedDecimal &other) noexcept : Numeric(other) {}

pepp::ast::SignedDecimal::SignedDecimal(SignedDecimal &&other) noexcept { swap(*this, other); }

std::string pepp::ast::SignedDecimal::string() const { return fmt::format("{:d}", static_cast<i64>(_value)); }

std::string pepp::ast::SignedDecimal::raw_string() const { return string(); }

pepp::ast::UnsignedDecimal::UnsignedDecimal() noexcept : Numeric() {}

pepp::ast::UnsignedDecimal::UnsignedDecimal(u64 value, u8 size) noexcept : Numeric(value, size) {}

pepp::ast::UnsignedDecimal::UnsignedDecimal(const UnsignedDecimal &other) noexcept : Numeric(other) {}

pepp::ast::UnsignedDecimal::UnsignedDecimal(UnsignedDecimal &&other) noexcept { swap(*this, other); }

std::string pepp::ast::UnsignedDecimal::string() const { return fmt::format("{:d}", static_cast<u64>(_value)); }

std::string pepp::ast::UnsignedDecimal::raw_string() const { return string(); }

pepp::ast::Hexadecimal::Hexadecimal() noexcept : Numeric() {}

pepp::ast::Hexadecimal::Hexadecimal(u64 value, u8 size) noexcept : Numeric(value, size) {}

pepp::ast::Hexadecimal::Hexadecimal(const Hexadecimal &other) noexcept : Numeric(other) {}

pepp::ast::Hexadecimal::Hexadecimal(Hexadecimal &&other) noexcept { swap(*this, other); }

std::string pepp::ast::Hexadecimal::string() const { return fmt::format("0x{:0{}X}", _value, 2 * _size); }

std::string pepp::ast::Hexadecimal::raw_string() const { return fmt::format("{:0{}X}", _value, 2 * _size); }
u64 pepp::ast::SignedDecimal::minimum_size() const noexcept {
  // Handle _value = 0b1000...0, otherwise we take log of negative number.
  if (_value * -1 == _value) return sizeof(_value);
  // Must subtract 1 bit (log2(n)+1), because the top order bit holds sign, not data.
  return ceil((log2(-1 * _value) + 1) / 8);
}
