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
#include <QSharedPointer>
#include <QtCore>
#include "types.hpp"

namespace symbol {
class Entry;
class Table;
} // namespace symbol
namespace symbol::value {
struct MaskedBits {
  quint8 byteCount = 0;
  quint64 bitPattern = 0, mask = 0;
  quint64 operator()();
  bool operator==(const MaskedBits &other) const;
};

/*!
 * \brief Provide a pure-virtual API for handling the various kinds of values
 * taken on by symbols.
 *
 * These various values include numeric constants, strings, and addresses in
 * memory. Since all of these types have different value types, a common API is
 * needed to allow them to act as drop-in replacements for each other.
 */
class Abstract {
private:
public:
  Abstract() = default;
  virtual ~Abstract() = default;
  /*!
   * \brief If the symbol points to code/data, it is the number of bytes needed
   * to fully represent that data. If the symbol is constant, it is the number
   * of bytes needed to hold the constant.
   */
  virtual quint32 size() const = 0;
  /*!
   * \brief Convert the internal value to MaskedBits, or throw an exception if
   * conversion is not possible.
   *
   * The symbol::value::Abstract::value method is the meat of the value API.
   * It allows us to store vastly different values (pointers, strings, integral
   * types) and provide a common API for these disparate types.
   *
   * \returns Returns the value interpreted as a MaskedValue.
   */
  virtual MaskedBits value() const = 0;
  /*!
   * \brief Describes the kind of object this value represents.
   * Possible examples include: static data, string data, address of a line of
   * code. While not important internally to our project, this field is required
   * by the ELF standard.
   *
   * \returns The kind of object represented by this value.
   * \sa symbol::Type
   */
  virtual Type type() const = 0;

  /*!
   * \brief Specify if the value specified by this symbol participates in
   * relocation. Constant values and pointer values cannot be relocated, but
   * addresses and relocation must be relocated.
   *
   * \returns True if the symbol participates in relocation, and false
   * otherwise.
   */
  virtual bool relocatable() const { return false; }

  /*!
   *
   * \returns A shallow copy of the derived class
   */
  virtual QSharedPointer<Abstract> clone() const = 0;
};

/*!
 * \brief Represent a value that is indefinite (not yet defined).
 */
class Empty : public Abstract {
public:
  explicit Empty();
  explicit Empty(quint8 bytes);
  Empty(const Empty &other);
  Empty(Empty &&other) noexcept;
  Empty &operator=(Empty other);
  friend void swap(Empty &first, Empty &second) {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    // gets data
    swap(first._bytes, second._bytes);
  }
  virtual ~Empty() override = default;

  quint32 size() const override;
  MaskedBits value() const override;
  Type type() const override;
  QSharedPointer<Abstract> clone() const override;

private:
  quint8 _bytes;
};

/*!
 * \brief The value taken on by a symbol that has been marked as deleted.
 *
 * A deleted value should never be propagated into an ELF binary--the linker
 * should error if deleted values are stuffed into the symbol table.
 */
class Deleted : public Abstract {
public:
  explicit Deleted();
  Deleted(Deleted &&other) noexcept;
  Deleted(const Deleted &other);
  Deleted &operator=(Deleted other);
  friend void swap(Deleted &, Deleted &) {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    // gets data.
  }
  virtual ~Deleted() override = default;

  quint32 size() const override;
  MaskedBits value() const override;
  symbol::Type type() const override;
  QSharedPointer<Abstract> clone() const override;
};

/*!
 * \brief Represent a value that is an integral constant.
 */
class Constant : public Abstract {
  MaskedBits _value;

public:
  explicit Constant();
  explicit Constant(MaskedBits value);
  Constant(const Constant &other);
  Constant(Constant &&other) noexcept;
  Constant &operator=(Constant other);
  friend void swap(Constant &first, Constant &second) {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    // gets data.
    swap(first._value, second._value);
  }
  virtual ~Constant() override = default;

  quint32 size() const override;
  MaskedBits value() const override;
  symbol::Type type() const override;
  QSharedPointer<Abstract> clone() const override;

  /*!
   * \brief Overwrite the internal value of this object using the given
   * parameters.
   *
   * \arg value The new value to be taken on by this object.
   */
  void setValue(MaskedBits value);
};

/*!
 * \brief Represent a value that has an address, such as a line of code.
 *
 * Effective addresses are computer by adding a base+offset.
 * Symbols of this value type participate in relocation, which allows the linker
 * to move the program around in memory. After creation, the base address is
 * immutable.
 */
class Location : public Abstract {

public:
  // Type defaults to kConstant, to avoid participation in relocation.
  explicit Location();
  // Type must be kCode or kObject.
  // pointedSize is the number of bytes stored at the pointer, while pointerSize
  // is the number of bytes to store the pointer. i.e., in pep10 `x:.block 65`
  // would have a pointedSize of 65 and a pointerSize of 2.
  explicit Location(quint16 pointedSize, quint16 pointerSize, quint64 base, quint64 offset, symbol::Type type);
  Location(const Location &other);
  Location(Location &&other) noexcept;
  Location &operator=(Location other);
  friend void swap(Location &first, Location &second) {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    // gets data.
    swap(first._pointerSize, second._pointerSize);
    swap(first._pointedSize, second._pointedSize);
    swap(first._base, second._base);
    swap(first._offset, second._offset);
    swap(first._type, second._type);
  }
  virtual ~Location() override = default;

  quint32 size() const override;
  // Inherited via value.
  virtual MaskedBits value() const override;
  symbol::Type type() const override;
  bool relocatable() const override;
  QSharedPointer<Abstract> clone() const override;

  /*!
   * \brief Increment the existing offset of this object.
   *
   * \arg value Value which will be added to the internal offset.
   */
  void addToOffset(quint64 value);
  /*!
   * \brief Clear and set the offset of this object.
   *
   * \arg value Value to which the this object's offest will be set
   */
  void setOffset(quint64 value);

  /*!
   * \brief Return this objects offset.
   *
   * \returns This object's offset.
   */
  quint64 offset() const;
  /*!
   * \brief Return this objects base address.
   *
   * \returns This object's base address.
   */
  quint64 base() const;

private:
  // Byte count
  quint16 _pointerSize, _pointedSize = 0;
  quint64 _base = 0, _offset = 0;
  symbol::Type _type = symbol::Type::kEmpty;
};

/*!
 * \brief Represent a value that take on the value of another symbol across
 * different symbol table hierachies.
 *
 * Used to reference symbols' values that are taken from other tables.
 *
 * This value cannot be relocated, since it acts like a numeric constant rather
 * than a location.
 */
class ExternalPointer : public Abstract {
public:
  explicit ExternalPointer(quint16 ptrSize);
  explicit ExternalPointer(quint16 ptrSize, QSharedPointer<symbol::Table> table,
                           QSharedPointer<const symbol::Entry> ptr);
  ExternalPointer(const ExternalPointer &other);
  ExternalPointer(ExternalPointer &&other) noexcept;
  ExternalPointer &operator=(ExternalPointer other);
  friend void swap(ExternalPointer &first, ExternalPointer &second) {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    // gets data.
    swap(first.symbol_pointer, second.symbol_pointer);
    swap(first.symbol_table, second.symbol_table);
    swap(first._ptrSize, second._ptrSize);
  }
  ~ExternalPointer() override = default;

  quint32 size() const override;
  // Inherited via value.
  MaskedBits value() const override;
  symbol::Type type() const override;
  QSharedPointer<Abstract> clone() const override;

  //! Symbol whose value is to be taken on. Does not need to belong to the same
  //! table. Does not need to have a common parent table.
  QSharedPointer<const symbol::Entry> symbol_pointer = {};
  QWeakPointer<symbol::Table> symbol_table;

private:
  quint16 _ptrSize;
};

/*!
 * \brief Represent a value that take on the value of another symbol within the
 * same symbol table hierachy.
 *
 * Used to reference symbols' values that are taken from other tables.
 *
 * This value cannot be relocated, since it acts like a numeric constant rather
 * than a location.
 */
class InternalPointer : public Abstract {
public:
  explicit InternalPointer(quint16 ptrSize);
  explicit InternalPointer(quint16 ptrSize, QSharedPointer<const symbol::Entry> ptr);
  InternalPointer(const InternalPointer &other);
  InternalPointer(InternalPointer &&other) noexcept;
  InternalPointer &operator=(InternalPointer other);
  friend void swap(InternalPointer &first, InternalPointer &second) {
    using std::swap;
    // swap((Abstract &)first, (Abstract &)second); // Add if data in base class
    // gets data.
    swap(first.symbol_pointer, second.symbol_pointer);
    swap(first._ptrSize, second._ptrSize);
  }
  ~InternalPointer() override = default;

  quint32 size() const override;
  // Inherited via value.
  MaskedBits value() const override;
  symbol::Type type() const override;
  QSharedPointer<Abstract> clone() const override;

  //! Symbol whose value is to be taken on. Does not need to belong to the same
  //! table, but must have a common parent table.
  QSharedPointer<const symbol::Entry> symbol_pointer = {};

private:
  quint16 _ptrSize;
};
}; // namespace symbol::value
