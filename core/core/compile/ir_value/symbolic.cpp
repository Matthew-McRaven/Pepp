#include "symbolic.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "spdlog/spdlog.h"

pepp::ast::Symbolic::Symbolic() {}

pepp::ast::Symbolic::Symbolic(u8 ptr_size, std::shared_ptr<core::symbol::Entry> value)
    : _ptr_size_bytes(ptr_size), _value(value) {}

pepp::ast::Symbolic::Symbolic(const Symbolic &other) : _ptr_size_bytes(other._ptr_size_bytes), _value(other._value) {}

pepp::ast::Symbolic::Symbolic(Symbolic &&other) noexcept { swap(*this, other); }

std::shared_ptr<pepp::core::symbol::Entry> pepp::ast::Symbolic::symbol() { return _value; }

std::shared_ptr<const pepp::core::symbol::Entry> pepp::ast::Symbolic::symbol() const { return _value; }

[[nodiscard]]
u32 pepp::ast::Symbolic::serialize(bits::span<u8> dest, bits::Order destEndian, u32 max_size) const noexcept {
  using size_type = bits::span<const u8>::size_type;
  const auto size = std::min<size_type>(max_size, serialized_size());
  if (_value == nullptr) {
    auto subspan = dest.subspan(0, size);
    std::fill(subspan.begin(), subspan.end(), 0);
  } else {
    auto src = _value->value->value();
    auto srcSpan = bits::span<const u8>{reinterpret_cast<const u8 *>(&src), static_cast<size_type>(size)};
    bits::memcpy_endian(dest, destEndian, srcSpan, bits::hostOrder());
  }
  return size;
}

std::string pepp::ast::Symbolic::string() const { return std::string(_value->name); }

std::string pepp::ast::Symbolic::raw_string() const { return string(); }
