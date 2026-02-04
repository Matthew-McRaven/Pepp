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

#pragma once
#include <memory>
#include "core/compile/symbol/types.hpp"
#include "core/math/bitmanip/mask.hpp"

namespace pepp::core::symbol {
class Entry;
class Table;

// Provide a pure-virtual API for handling the various kinds of symbolic values
class AbstractValue {
private:
public:
  AbstractValue() noexcept = default;
  virtual ~AbstractValue() noexcept = default;

  // For constants, the minimum number of bytes to hold a constant.
  // For code/data, the number of bytes in the pointed-to object.
  virtual u32 size() const noexcept = 0;

  // Converts internal value to MaskedBits, or throws an exception if not possible
  virtual bits::MaskedBits value() const noexcept = 0;
  // Describe the kind of object this value represents for ease of translation to ELF
  virtual Type type() const noexcept = 0;
};

// An undefined symbol, possible occupying some number of bytes.
class EmptyValue : public AbstractValue {
public:
  explicit EmptyValue() noexcept;
  explicit EmptyValue(u8 bytes) noexcept;
  EmptyValue(const EmptyValue &other) noexcept;
  EmptyValue(EmptyValue &&other) noexcept;
  EmptyValue &operator=(EmptyValue other);
  friend void swap(EmptyValue &first, EmptyValue &second) noexcept {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    swap(first._bytes, second._bytes);
  }
  virtual ~EmptyValue() noexcept override = default;

  u32 size() const noexcept override;
  bits::MaskedBits value() const noexcept override;
  Type type() const noexcept override;

private:
  u8 _bytes;
};

// Value given to a symbol marked as deleted but not yet destroyed (e.g., still a live reference to the symbol via
// shared_ptr). Attempts to access a deleted symbol's value should either: succed and issue a warning to CERR, or throw
// an exception. This type has no equivalent in ELF -- it only exists because of our reference counting scheme.
class DeletedValue : public AbstractValue {
public:
  explicit DeletedValue() noexcept;
  DeletedValue(DeletedValue &&other) noexcept;
  DeletedValue(const DeletedValue &other) noexcept;
  DeletedValue &operator=(DeletedValue other) noexcept;
  friend void swap(DeletedValue &, DeletedValue &) noexcept {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
  }
  virtual ~DeletedValue() override = default;

  u32 size() const noexcept override;
  bits::MaskedBits value() const noexcept override;
  symbol::Type type() const noexcept override;
};

// Integral constant
class ConstantValue : public AbstractValue {
  bits::MaskedBits _value;

public:
  explicit ConstantValue() noexcept;
  explicit ConstantValue(bits::MaskedBits value) noexcept;
  ConstantValue(const ConstantValue &other) noexcept;
  ConstantValue(ConstantValue &&other) noexcept;
  ConstantValue &operator=(ConstantValue other) noexcept;
  friend void swap(ConstantValue &first, ConstantValue &second) noexcept {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    swap(first._value, second._value);
  }
  virtual ~ConstantValue() noexcept override = default;

  u32 size() const noexcept override;
  bits::MaskedBits value() const noexcept override;
  symbol::Type type() const noexcept override;

  // Overwrite the internal value of this instance
  void set_value(bits::MaskedBits value) noexcept;
};

// A value that has an address, such as a global allocation or a line of code.
// Effective address is base+offset. Most symbols of this type participate in relocation.
// Base address is immutable after creation.
class LocationValue : public AbstractValue {

public:
  // Type defaults to kConstant, to avoid participation in relocation.
  explicit LocationValue();
  // Type must be kCode or kObject.
  // pointed_size is the number of bytes stored at the pointer, while pointer_size
  // is the number of bytes to store the pointer. i.e., in pep10 `x:.block 65`
  // would have a pointedSize of 65 and a pointerSize of 2.
  explicit LocationValue(u16 pointed_size, u16 pointer_size, u64 base, u64 offset, symbol::Type type) noexcept;
  LocationValue(const LocationValue &other) noexcept;
  LocationValue(LocationValue &&other) noexcept;
  LocationValue &operator=(LocationValue other) noexcept;
  friend void swap(LocationValue &first, LocationValue &second) noexcept {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    swap(first._pointer_size, second._pointer_size);
    swap(first._pointed_size, second._pointed_size);
    swap(first._base, second._base);
    swap(first._offset, second._offset);
    swap(first._type, second._type);
  }
  virtual ~LocationValue() noexcept override = default;

  u32 size() const noexcept override;
  // Inherited via value.
  bits::MaskedBits value() const noexcept override;
  symbol::Type type() const noexcept override;

  // Increment existing offset
  void increment_offset(u64 value) noexcept;
  // Overwrite existing offset
  void set_offset(u64 value) noexcept;

  // Returns this object's offset from base address.
  u64 offset() const noexcept;
  // Returns this object's base address.
  u64 base() const noexcept;
  u64 effective_address() const noexcept;

private:
  // Byte count
  u16 _pointer_size, _pointed_size = 0;
  u64 _base = 0, _offset = 0;
  symbol::Type _type = Type::Empty;
};

/*!
 * \brief Represent a value that take on the value of another symbol within the current symbol table
 *
 * This value cannot be relocated, since it acts like a numeric constant rather
 * than a location.
 */
class AliasValue : public AbstractValue {
public:
  explicit AliasValue(u16 ptr_size) noexcept;
  explicit AliasValue(u16 ptr_size, std::shared_ptr<const symbol::Entry> ptr) noexcept;
  AliasValue(const AliasValue &other) noexcept;
  AliasValue(AliasValue &&other) noexcept;
  AliasValue &operator=(AliasValue other) noexcept;
  friend void swap(AliasValue &first, AliasValue &second) noexcept {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    swap(first.symbol_pointer, second.symbol_pointer);
    swap(first._ptr_size, second._ptr_size);
  }
  ~AliasValue() noexcept override = default;

  u32 size() const noexcept override;
  bits::MaskedBits value() const noexcept override;
  symbol::Type type() const noexcept override;

  std::shared_ptr<const Entry> symbol_pointer = {};

private:
  u16 _ptr_size;
};
}; // namespace pepp::core::symbol
