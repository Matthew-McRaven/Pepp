/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
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
#pragma once
#include <memory>
#include "core/compile/abstract_value/base.hpp"

namespace pepp::core::symbol {
class Entry;
}
namespace pepp::ast {

// Represents a value that propogates the value of another symbol within the current table.
// This value cannot be relocated, since it acts like a numeric constant rather than a location.
// This class provides no API for detecting or prevent binding loops.
struct Symbolic : public BaseValue {
public:
  explicit Symbolic();
  Symbolic(u16 ptr_size, std::shared_ptr<core::symbol::Entry> value);
  Symbolic(const Symbolic &other);
  Symbolic(Symbolic &&other) noexcept;
  Symbolic &operator=(Symbolic other);
  friend void swap(Symbolic &first, Symbolic &second) {
    using std::swap;
    swap(first._ptr_size, second._ptr_size);
    swap(first._value, second._value);
  }
  std::shared_ptr<core::symbol::Entry> symbol();
  std::shared_ptr<const core::symbol::Entry> symbol() const;
  inline bool numeric() const noexcept override { return true; }
  inline bool signed_numeric() const noexcept override { return false; }
  inline bool text() const noexcept override { return false; }
  inline bool identifier() const noexcept override { return true; }
  inline bool wide() const noexcept override { return false; }
  inline bool fixed_size() const noexcept override { return true; }
  u64 stream_size() const noexcept override;
  inline void set_stream_size(u64 size) noexcept override { _ptr_size = size; }
  u64 min_size() const noexcept override;
  void value(bits::span<u8> dest, bits::Order targetEndian = bits::hostOrder()) const noexcept override;

  std::string string() const override;
  std::string raw_string() const override;

private:
  u16 _ptr_size;
  std::shared_ptr<core::symbol::Entry> _value = nullptr;
};

// Value given to a symbol marked as deleted but not yet destroyed (e.g., still a live reference to the symbol via
// shared_ptr). Attempts to access a deleted symbol's value should either: succed and issue a warning to CERR, or throw
// an exception. This type has no equivalent in ELF -- it only exists because of our reference counting scheme.
struct Deleted : public BaseValue {
  explicit Deleted();
  Deleted(const Deleted &other);
  Deleted(Deleted &&other) noexcept;
  Deleted &operator=(Deleted other);
  friend void swap(Deleted &first, Deleted &second) { using std::swap; }
  inline bool numeric() const noexcept override { return false; }
  inline bool signed_numeric() const noexcept override { return false; }
  inline bool text() const noexcept override { return false; }
  inline bool identifier() const noexcept override { return false; }
  inline bool wide() const noexcept override { return false; }
  inline bool fixed_size() const noexcept override { return true; }
  u64 stream_size() const noexcept override { return 0; };
  inline void set_stream_size(u64 size) noexcept override {}
  u64 min_size() const noexcept override { return 0; }
  void value(bits::span<u8> dest, bits::Order targetEndian = bits::hostOrder()) const noexcept override;

  inline std::string string() const override { return ""; }
  inline std::string raw_string() const override { return ""; }
};

} // namespace pepp::ast
