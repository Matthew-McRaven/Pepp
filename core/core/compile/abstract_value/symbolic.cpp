#include "symbolic.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "spdlog/spdlog.h"

pepp::ast::Symbolic::Symbolic() {}

pepp::ast::Symbolic::Symbolic(u16 ptr_size, std::shared_ptr<core::symbol::Entry> value)
    : _ptr_size(ptr_size), _value(value) {}

pepp::ast::Symbolic::Symbolic(const Symbolic &other) : _ptr_size(other._ptr_size), _value(other._value) {}

pepp::ast::Symbolic::Symbolic(Symbolic &&other) noexcept { swap(*this, other); }

std::shared_ptr<pepp::core::symbol::Entry> pepp::ast::Symbolic::symbol() { return _value; }

std::shared_ptr<const pepp::core::symbol::Entry> pepp::ast::Symbolic::symbol() const { return _value; }

u64 pepp::ast::Symbolic::stream_size() const noexcept { return _ptr_size; }

u64 pepp::ast::Symbolic::min_size() const noexcept { return ceil(log2(_value->value->value()() + 1) / 8); }

void pepp::ast::Symbolic::value(bits::span<u8> dest, bits::Order destEndian) const noexcept {
  if (_value == nullptr) {
    auto subspan = dest.subspan(0, stream_size());
    std::fill(subspan.begin(), subspan.end(), 0);
  } else {
    auto src = _value->value->value();
    using size_type = bits::span<const u8>::size_type;
    auto srcSpan = bits::span<const u8>{reinterpret_cast<const u8 *>(&src), static_cast<size_type>(stream_size())};
    bits::memcpy_endian(dest, destEndian, srcSpan, bits::hostOrder());
  }
}

std::string pepp::ast::Symbolic::string() const { return std::string(_value->name); }

std::string pepp::ast::Symbolic::raw_string() const { return string(); }

pepp::ast::Deleted::Deleted() {}

pepp::ast::Deleted::Deleted(const Deleted &other) {}

pepp::ast::Deleted::Deleted(Deleted &&other) noexcept { swap(*this, other); }

void pepp::ast::Deleted::value(bits::span<u8> dest, bits::Order targetEndian) const noexcept {
  SPDLOG_WARN("Attempting to access value of pepp::core::symbol::DeletedValue");
}
