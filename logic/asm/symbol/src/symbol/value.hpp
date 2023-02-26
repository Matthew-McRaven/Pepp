#pragma once

// File: value.hpp
/*
    The Pep/10 suite of applications (Pep10, Pep10CPU, Pep10Term) are
    simulators for the Pep/10 virtual machine, and allow users to
    create, simulate, and debug across various levels of abstraction.

    Copyright (C) 2019-2023 J. Stanley Warford & Matthew McRaven, Pepperdine
   University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QSharedPointer>
#include <QtCore>

#include "symbol_globals.hpp"
#include "types.hpp"

namespace symbol {
class Entry;
}
namespace symbol::value {
struct SYMBOL_EXPORT MaskedBits {
  quint8 byteCount;
  quint64 bitPattern, mask;
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
class SYMBOL_EXPORT Abstract {
private:
public:
  Abstract() = default;
  virtual ~Abstract() = default;
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
};

/*!
 * \brief Represent a value that is indefinite (not yet defined).
 */
class SYMBOL_EXPORT Empty : public Abstract {
public:
  explicit Empty(quint8 bytes);
  virtual ~Empty() override = default;
  MaskedBits value() const override;
  Type type() const override;

private:
  quint8 _bytes;
};

/*!
 * \brief The value taken on by a symbol that has been marked as deleted.
 *
 * A deleted value should never be propagated into an ELF binary--the linker
 * should error if deleted values are stuffed into the symbol table.
 */
class SYMBOL_EXPORT Deleted : public Abstract {
public:
  Deleted() = default;
  virtual ~Deleted() override = default;
  MaskedBits value() const override;
  symbol::Type type() const override;
};

/*!
 * \brief Represent a value that is an integral constant.
 */
class SYMBOL_EXPORT Constant : public Abstract {
  MaskedBits _value;

public:
  explicit Constant(MaskedBits value);
  virtual ~Constant() override = default;
  MaskedBits value() const override;
  symbol::Type type() const override;

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
class SYMBOL_EXPORT Location : public Abstract {

public:
  // Type must be kCode or kObject.
  explicit Location(quint8 bytes, quint64 base, quint64 offset,
                    symbol::Type type);
  virtual ~Location() override = default;
  // Inherited via value.
  virtual MaskedBits value() const override;
  symbol::Type type() const override;
  bool relocatable() const override;

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
  quint8 _bytes;
  quint64 _base, _offset;
  symbol::Type _type;
};

/*!
 * \brief Represent a value that take on the value of another symbol.
 *
 * Used to reference symbols' values that are taken from other tables.
 *
 * This value cannot be relocated, since it acts like a numeric constant rather
 * than a location.
 */
class SYMBOL_EXPORT Pointer : public Abstract {
public:
  explicit Pointer(QSharedPointer<const symbol::Entry> ptr);
  ~Pointer() override = default;
  // Inherited via value.
  MaskedBits value() const override;
  symbol::Type type() const override;

  //! Symbol whose value is to be taken on. Does not need to belong to the same
  //! table.
  QSharedPointer<const symbol::Entry> symbol_pointer;
};
}; // namespace symbol::value
