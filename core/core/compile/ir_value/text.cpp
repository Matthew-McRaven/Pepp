#include "text.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/strings.hpp"
#include "fmt/format.h"

pepp::ast::String::String() = default;

pepp::ast::String::String(std::string value) {
  auto start = value.begin();
  bool okay = true;
  uint8_t temp = 0;
  while (start != value.end()) {
    okay &= bits::charactersToByte(start, value.end(), temp);
    _bytes.push_back(temp);
  }
  _size = _bytes.size();

  if (!okay) {
    static const char *const e = "Invalid escape sequence in string";
    std::cerr << e;
    throw std::logic_error(e);
  }
}

pepp::ast::String::String(const String &other) : _size(other._size), _bytes(other._bytes) {}

pepp::ast::String::String(String &&other) noexcept { swap(*this, other); }

[[nodiscard]]
u32 pepp::ast::String::serialize(bits::span<u8> dest, bits::Order destEndian, u32 max_size) const noexcept {
  using size_type = bits::span<const u8>::size_type;
  const auto size = std::min<size_type>(max_size, serialized_size());
  std::span<const u8> src(reinterpret_cast<const u8 *>(_bytes.data()), size);
  bits::memcpy_endian(dest, destEndian, src, bits::hostOrder());
  return size;
}

std::string pepp::ast::String::string() const { return fmt::format("\"{}\"", raw_string()); }

std::string pepp::ast::String::raw_string() const {
  std::string out;
  for (auto byte : _bytes) bits::byteToEscaped(byte, std::back_inserter(out));
  return out;
}

pepp::ast::Character::Character() = default;

pepp::ast::Character::Character(char value) : _value(value) {}

pepp::ast::Character::Character(const Character &other) : _value(other._value) {}

pepp::ast::Character::Character(Character &&other) noexcept { swap(*this, other); }

[[nodiscard]]
u32 pepp::ast::Character::serialize(bits::span<u8> dest, bits::Order destEndian, u32 max_size) const noexcept {
  using size_type = bits::span<const u8>::size_type;
  const auto size = std::min<size_type>(max_size, 1);
  std::span<const u8> src((u8 *)&_value, size);
  bits::memcpy_endian(dest, destEndian, src, bits::hostOrder());
  return size;
}

std::string pepp::ast::Character::string() const { return fmt::format("'{}'", raw_string()); }

std::string pepp::ast::Character::raw_string() const {
  std::string out;
  bits::byteToEscaped(_value, std::back_inserter(out));
  return out;
}
