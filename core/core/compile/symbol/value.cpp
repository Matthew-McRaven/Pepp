/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "core/compile/symbol/value.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "core/compile/symbol/entry.hpp"

pepp::core::symbol::EmptyValue::EmptyValue() noexcept : _bytes(0) {}

pepp::core::symbol::EmptyValue::EmptyValue(u8 bytes) noexcept : _bytes(bytes) {}

pepp::core::symbol::EmptyValue::EmptyValue(const EmptyValue &other) noexcept : _bytes(other._bytes) {}

pepp::core::symbol::EmptyValue::EmptyValue(EmptyValue &&other) noexcept { swap(*this, other); }

pepp::core::symbol::EmptyValue &pepp::core::symbol::EmptyValue::operator=(EmptyValue other) {
  swap(*this, other);
  return *this;
}

u32 pepp::core::symbol::EmptyValue::size() const noexcept { return 0; }

bits::MaskedBits pepp::core::symbol::EmptyValue::value() const noexcept {
  return {.byteCount = _bytes, .bitPattern = 0, .mask = 0x0};
}

pepp::core::symbol::Type pepp::core::symbol::EmptyValue::type() const noexcept {
  return pepp::core::symbol::Type::Empty;
}

pepp::core::symbol::DeletedValue::DeletedValue() noexcept {}

pepp::core::symbol::DeletedValue::DeletedValue(DeletedValue &&other) noexcept { swap(*this, other); }

pepp::core::symbol::DeletedValue::DeletedValue(const DeletedValue &other) noexcept {}

pepp::core::symbol::DeletedValue &pepp::core::symbol::DeletedValue::operator=(DeletedValue other) noexcept {
  swap(*this, other);
  return *this;
}

u32 pepp::core::symbol::DeletedValue::size() const noexcept { return 0; }

bits::MaskedBits pepp::core::symbol::DeletedValue::value() const noexcept {
  SPDLOG_WARN("Attempting to access value of pepp::core::symbol::DeletedValue");
  return {.byteCount = 0, .bitPattern = 0, .mask = 0x0};
}

pepp::core::symbol::Type pepp::core::symbol::DeletedValue::type() const noexcept { return Type::Deleted; }

pepp::core::symbol::ConstantValue::ConstantValue() noexcept : _value({}) {}

pepp::core::symbol::ConstantValue::ConstantValue(bits::MaskedBits value) noexcept : _value(value) {}

pepp::core::symbol::ConstantValue::ConstantValue(const ConstantValue &other) noexcept : _value(other._value) {}

pepp::core::symbol::ConstantValue::ConstantValue(ConstantValue &&other) noexcept { swap(*this, other); }

pepp::core::symbol::ConstantValue &pepp::core::symbol::ConstantValue::operator=(ConstantValue other) noexcept {
  swap(*this, other);
  return *this;
}

u32 pepp::core::symbol::ConstantValue::size() const noexcept { return _value.byteCount; }

bits::MaskedBits pepp::core::symbol::ConstantValue::value() const noexcept { return _value; }

pepp::core::symbol::Type pepp::core::symbol::ConstantValue::type() const noexcept { return Type::Constant; }

void pepp::core::symbol::ConstantValue::set_value(bits::MaskedBits value) noexcept { this->_value = value; }

pepp::core::symbol::LocationValue::LocationValue() {}

pepp::core::symbol::LocationValue::LocationValue(u16 pointed_size, u16 pointer_size, u64 base, u64 offset,
                                                 Type type) noexcept
    : _pointed_size(pointed_size), _pointer_size(pointer_size), _base(base), _offset(offset), _type(type) {
  switch (type) {
  case Type::Object: [[fallthrough]];
  case Type::Code: break;
  default:
    SPDLOG_CRITICAL("Invalid LocationValue type passed to pepp::core::symbol::LocationValue, {}", (int)type);
    std::terminate();
  }
}

pepp::core::symbol::LocationValue::LocationValue(const LocationValue &other) noexcept
    : _pointer_size(other._pointer_size), _pointed_size(other._pointed_size), _base(other._base),
      _offset(other._offset), _type(other._type) {}

pepp::core::symbol::LocationValue::LocationValue(LocationValue &&other) noexcept { swap(*this, other); }

pepp::core::symbol::LocationValue &pepp::core::symbol::LocationValue::operator=(LocationValue other) noexcept {
  swap(*this, other);
  return *this;
}

u32 pepp::core::symbol::LocationValue::size() const noexcept { return _pointed_size; }

bits::MaskedBits pepp::core::symbol::LocationValue::value() const noexcept {
  return {.byteCount = static_cast<u8>(_pointer_size),
          .bitPattern = effective_address(),
          .mask = bits::mask(_pointer_size)};
}

pepp::core::symbol::Type pepp::core::symbol::LocationValue::type() const noexcept { return _type; }

void pepp::core::symbol::LocationValue::increment_offset(u64 value) noexcept { _offset += value; }

void pepp::core::symbol::LocationValue::set_offset(u64 value) noexcept { _offset = value; }

u64 pepp::core::symbol::LocationValue::offset() const noexcept { return _offset; }

u64 pepp::core::symbol::LocationValue::base() const noexcept { return _base; }

u64 pepp::core::symbol::LocationValue::effective_address() const noexcept { return _base + _offset; }

pepp::core::symbol::AliasValue::AliasValue(u16 ptrSize) noexcept : _ptr_size(ptrSize) {}

pepp::core::symbol::AliasValue::AliasValue(u16 ptr_size, std::shared_ptr<const Entry> ptr) noexcept
    : _ptr_size(ptr_size), symbol_pointer(ptr) {}

pepp::core::symbol::AliasValue::AliasValue(const AliasValue &other) noexcept
    : _ptr_size(other._ptr_size), symbol_pointer(other.symbol_pointer) {}

pepp::core::symbol::AliasValue::AliasValue(AliasValue &&other) noexcept { swap(*this, other); }

pepp::core::symbol::AliasValue &pepp::core::symbol::AliasValue::operator=(AliasValue other) noexcept {
  swap(*this, other);
  return *this;
}

u32 pepp::core::symbol::AliasValue::size() const noexcept { return _ptr_size; }

bits::MaskedBits pepp::core::symbol::AliasValue::value() const noexcept {
  if (symbol_pointer == nullptr) return {.byteCount = 0, .bitPattern = 0, .mask = 0};
  return symbol_pointer->value->value();
}

pepp::core::symbol::Type pepp::core::symbol::AliasValue::type() const noexcept { return Type::Alias; }
