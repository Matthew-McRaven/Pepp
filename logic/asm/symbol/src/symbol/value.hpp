#pragma once

// File: value.hpp
/*
    The Pep/10 suite of applications (Pep10, Pep10CPU, Pep10Term) are
    simulators for the Pep/10 virtual machine, and allow users to
    create, simulate, and debug across various levels of abstraction.

    Copyright (C) 2021 J. Stanley Warford & Matthew McRaven, Pepperdine University

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

#include <climits>
#include <memory>

#include "types.hpp"

namespace symbol {
template<typename value_t> class entry;

/*!
 * \brief Provide a pure-virtual API for handling the various kinds of values taken on by symbols.
 *
 * These various valuse include numeric constants, strings, and addresses in memory.
 * Since all of these types have different value types, a common API is needed to allow them to act as drop-in
 * replacements for each other.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template<typename value_t = uint16_t> class abstract_value {
private:
public:
  abstract_value() = default;
  virtual ~abstract_value() = default;
  /*!
   * \brief Convert the internal value to a value_t, or throw an exception if conversion is not possible.
   *
   * The symbol::abstract_value::value method is the meat of the value API.
   * It allows us to store vastly different values (pointers, strings, integral types) and provide a common API for
   * these disparate types.
   *
   * \returns Returns the value interpereted as a value_t.
   */
  virtual value_t value() const = 0;
  /*!
   * \brief Describes the kind of object this value represents.
   * Possible examples include: static data, string data, address of a line of code.
   * While not important internally to our project, this field is required by the ELF standard.
   *
   * \returns The kind of object represented by this value.
   * \sa symbol::type
   */
  virtual Type type() const = 0;

  /*!
   * \brief Return the number of bytes needed to hold the symbol's value.
   * Very often it is sizeof(value_t), but that may not be the case for variable length strings.
   *
   * \returns The number of bytes needed to hold the result of converting this classes value to an integral type.
   */
  virtual size_t size() const { return 2; }

  /*!
   * \brief Specify if the value specified by this symbol participates in relocation.
   * Constant values and pointer values cannot be relocated, but addresses and relocation must be relocated.
   *
   * \returns True if the symbol participates in relocation, and false otherwise.
   */
  virtual bool relocatable() const { return false; }
};

/*!
 * \brief Represent a value that is indefinite (not yet defined).
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template<typename value_t = uint16_t> class value_empty : public abstract_value<value_t> {
public:
  value_empty();
  virtual ~value_empty() override = default;
  value_t value() const override;
  Type type() const override;
};

/*!
 * \brief The value taken on by a symbol that has been marked as deleted.
 *
 * A deleted value should never be propagated into an ELF binary--the linker should error if deleted values are stuffed
 * into the symbol table.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template<typename value_t = uint16_t> class value_deleted : public value_empty<value_t> {
public:
  value_deleted() = default;
  virtual ~value_deleted() override = default;
  symbol::Type type() const override;
};

/*!
 * \brief Represent a value that is an integral constant.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template<typename value_t = uint16_t> class value_const : public abstract_value<value_t> {
  value_t value_;

public:
  explicit value_const(value_t value);
  virtual ~value_const() override = default;
  value_t value() const override;
  symbol::Type type() const override;

  /*!
   * \brief Overwrite the internal value of this object using the given parameters.
   *
   * \arg value_t The new value to be taken on by this object.
   */
  void set_value(value_t);
};

/*!
 * \brief Represent a value that has an address, such as a line of code.
 *
 * Effective addresses are computer by adding a base+offset.
 * Symbols of this value type participate in relocation, which allows the linker to move the program around in memory.
 * After creation, the base address is immutable.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template<typename value_t = uint16_t> class value_location : public abstract_value<value_t> {
  value_t base_, offset_;

public:
  // Type must be kCode or kObject.
  explicit value_location(value_t base, value_t offset, symbol::Type type);
  virtual ~value_location() override = default;
  // Inherited via value.
  virtual value_t value() const override;
  symbol::Type type() const override;
  bool relocatable() const override;

  /*!
   * \brief Increment the existing offset of this object.
   *
   * \arg value Value which will be added to the internal offset.
   */
  void add_to_offset(value_t value);
  /*!
   * \brief Clear and set the offset of this object.
   *
   * \arg value Value to which the this object's offest will be set
   */
  void set_offset(value_t value);

  /*!
   * \brief Return this objects offset.
   *
   * \returns This object's offset.
   */
  value_t offset() const;
  /*!
   * \brief Return this objects base address.
   *
   * \returns This object's base address.
   */
  value_t base() const;

private:
  symbol::Type type_;
};

/*!
 * \brief Represent a value that take on the value of another symbol.
 *
 * Used to reference symbols' values that are taken from other tables.
 *
 * This value cannot be relocated, since it acts like a numeric constant rather than a location.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template<typename value_t = uint16_t> class value_pointer : public abstract_value<value_t> {
public:
  explicit value_pointer(std::shared_ptr<const entry<value_t>>);
  ~value_pointer() override = default;
  // Inherited via value.
  value_t value() const override;
  symbol::Type type() const override;

  //! Symbol whose value is to be taken on. Does not need to belong to the same table.
  std::shared_ptr<const entry<value_t>> symbol_pointer;
};
}; // end namespace symbol

#include "value.tpp"