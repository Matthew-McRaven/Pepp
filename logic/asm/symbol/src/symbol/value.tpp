// File: value.tpp
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

#include <cassert>

#include "entry.hpp"

template <typename value_t> symbol::value_empty<value_t>::value_empty() : abstract_value<value_t>() {}

template <typename value_t> value_t symbol::value_empty<value_t>::value() const { return value_t(); }

template <typename value_t> symbol::Type symbol::value_empty<value_t>::type() const { return Type::kEmpty; }

template <typename value_t> symbol::Type symbol::value_deleted<value_t>::type() const { return Type::kDeleted; }

template <typename value_t>
symbol::value_const<value_t>::value_const(value_t value) : abstract_value<value_t>(), value_(value) {}

template <typename value_t> value_t symbol::value_const<value_t>::value() const { return value_; }

template <typename value_t> void symbol::value_const<value_t>::set_value(value_t value) { value_ = value; }

template <typename value_t> symbol::Type symbol::value_const<value_t>::type() const { return Type::kConstant; }

template <typename value_t>
symbol::value_location<value_t>::value_location(value_t base, value_t offset, symbol::Type type)
    : abstract_value<value_t>(), base_(base), offset_(offset), type_(type) {
    assert(type == symbol::Type::kCode || type == symbol::Type::kObject);
}

template <typename value_t> void symbol::value_location<value_t>::set_offset(value_t value) { this->offset_ = value; }

template <typename value_t> void symbol::value_location<value_t>::add_to_offset(value_t value) {
    this->offset_ += value;
}

template <typename value_t> value_t symbol::value_location<value_t>::value() const { return base_ + offset_; }

template <typename value_t> symbol::Type symbol::value_location<value_t>::type() const { return type_; }

template <typename value_t> bool symbol::value_location<value_t>::relocatable() const { return true; }

template <typename value_t> value_t symbol::value_location<value_t>::offset() const { return offset; }

template <typename value_t> value_t symbol::value_location<value_t>::base() const { return base; }

template <typename value_t>
symbol::value_pointer<value_t>::value_pointer(std::shared_ptr<const entry<value_t>> symbol)
    : abstract_value<value_t>(), symbol_pointer(std::move(symbol)) {}

template <typename value_t> value_t symbol::value_pointer<value_t>::value() const {
    return symbol_pointer->value->value();
}

template <typename value_t> symbol::Type symbol::value_pointer<value_t>::type() const { return Type::kPtrToSym; }
