#include "text.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/strings.hpp"
#include "fmt/format.h"

pepp::ast::String::String() {}

pepp::ast::String::String(std::string value) {
  auto start = value.begin();
  bool okay = true;
  uint8_t temp = 0;
  while (start != value.end()) {
    okay &= bits::charactersToByte(start, value.end(), temp);
    _bytes.push_back(temp);
  }

  if (!okay) {
    static const char *const e = "Invalid escape sequence in string";
    std::cerr << e;
    throw std::logic_error(e);
  }
}

pepp::ast::String::String(const String &other) : _size(other._size), _bytes(other._bytes) {}

pepp::ast::String::String(String &&other) noexcept { swap(*this, other); }

void pepp::ast::String::value(bits::span<u8> dest, bits::Order destEndian) const noexcept {
  using size_type = bits::span<const u8>::size_type;
  bits::memcpy_endian(
      dest, destEndian,
      bits::span<const u8>{reinterpret_cast<const u8 *>(_bytes.data()), static_cast<size_type>(stream_size())},
      bits::hostOrder());
}

std::string pepp::ast::String::string() const { return fmt::format("\"{}\"", raw_string()); }

std::string pepp::ast::String::raw_string() const {
  std::string out;
  for (auto byte : _bytes) bits::byteToEscaped(byte, std::back_inserter(out));
  return out;
}

pepp::ast::Character::Character() {}

pepp::ast::Character::Character(char value) : _value(value) {}

pepp::ast::Character::Character(const Character &other) {}

pepp::ast::Character::Character(Character &&other) noexcept {}

void pepp::ast::Character::value(bits::span<u8> dest, bits::Order destEndian) const noexcept {
  using size_type = bits::span<const u8>::size_type;
  std::span<u8> chars((u8 *)&_value, 1);
  bits::memcpy_endian(dest, destEndian, chars, destEndian);
}

std::string pepp::ast::Character::string() const { return fmt::format("'{}'", raw_string()); }

std::string pepp::ast::Character::raw_string() const {
  std::string out;
  bits::byteToEscaped(_value, std::back_inserter(out));
  return out;
}
