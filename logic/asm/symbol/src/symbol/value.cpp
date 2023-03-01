// File: value.cpp
/*
    The Pep/10 suite of applications (Pep10, Pep10CPU, Pep10Term) are
    simulators for the Pep/10 virtual machine, and allow users to
    create, simulate, and debug across various levels of abstraction.

    Copyright (C) 2021 J. Stanley Warford & Matthew McRaven, Pepperdine
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
#include "value.hpp"
#include <QDebug>

#include "entry.hpp"

symbol::value::Empty::Empty() : _bytes(0) {}

symbol::value::Empty::Empty(quint8 bytes) : _bytes(bytes) {}

symbol::value::Empty::Empty(const Empty &other) : _bytes(other._bytes) {}

symbol::value::Empty::Empty(Empty &&other) noexcept { swap(*this, other); }

symbol::value::Empty &symbol::value::Empty::operator=(Empty other) {
  swap(*this, other);
  return *this;
}

symbol::value::MaskedBits symbol::value::Empty::value() const {
  return {.byteCount = _bytes, .bitPattern = 0, .mask = 0x0};
}

symbol::Type symbol::value::Empty::type() const { return symbol::Type::kEmpty; }

QSharedPointer<symbol::value::Abstract> symbol::value::Empty::clone() const {
  return QSharedPointer<Empty>::create(*this);
}

symbol::value::Deleted::Deleted()
{

}

symbol::value::Deleted::Deleted(Deleted &&other) noexcept {
  swap(*this, other);
}

symbol::value::Deleted::Deleted(const Deleted &other) {}

symbol::value::Deleted &symbol::value::Deleted::operator=(Deleted other) {
  swap(*this, other);
  return *this;
}

symbol::value::MaskedBits symbol::value::Deleted::value() const {
  qWarning() << "Attempting to access value of symbol::value::Deleted";
  return {.byteCount = 0, .bitPattern = 0, .mask = 0x0};
}

symbol::Type symbol::value::Deleted::type() const {
  return symbol::Type::kDeleted;
}

QSharedPointer<symbol::value::Abstract> symbol::value::Deleted::clone() const {
  return QSharedPointer<Deleted>::create(*this);
}

symbol::value::Constant::Constant() : _value({}) {}

symbol::value::Constant::Constant(MaskedBits value) : _value(value) {}

symbol::value::Constant::Constant(const Constant &other)
    : _value(other._value) {}

symbol::value::Constant::Constant(Constant &&other) noexcept {
  swap(*this, other);
}

symbol::value::Constant &symbol::value::Constant::operator=(Constant other) {
  swap(*this, other);
  return *this;
}

symbol::value::MaskedBits symbol::value::Constant::value() const {
  return _value;
}

symbol::Type symbol::value::Constant::type() const {
  return symbol::Type::kConstant;
}

QSharedPointer<symbol::value::Abstract> symbol::value::Constant::clone() const {
  return QSharedPointer<Constant>::create(*this);
}

void symbol::value::Constant::setValue(MaskedBits value) {
  this->_value = value;
}

symbol::value::Location::Location() {}

symbol::value::Location::Location(quint8 bytes, quint64 base, quint64 offset,
                                  Type type)
    : _bytes(bytes), _base(base), _offset(offset) {
  switch (type) {
  case symbol::Type::kObject:
    [[fallthrough]];
  case symbol::Type::kCode:
    break;
  default:
    qCritical() << "Invalid Location type passed to symbol::value::Location"
                << type;
  }
}

symbol::value::Location::Location(const Location &other)
    : _bytes(other._bytes), _base(other._base), _offset(other._offset),
      _type(other._type) {}

symbol::value::Location::Location(Location &&other) noexcept {
  swap(*this, other);
}

symbol::value::Location &symbol::value::Location::operator=(Location other) {
  swap(*this, other);
  return *this;
}

symbol::value::MaskedBits symbol::value::Location::value() const {
  return {.byteCount = _bytes,
          .bitPattern = _base + _offset,
          // Computing bitmask: https://stackoverflow.com/a/1392065
          .mask = (8ul << _bytes) - 1};
}

symbol::Type symbol::value::Location::type() const { return _type; }

bool symbol::value::Location::relocatable() const { return true; }

QSharedPointer<symbol::value::Abstract> symbol::value::Location::clone() const {
  return QSharedPointer<Location>::create(*this);
}

void symbol::value::Location::addToOffset(quint64 value) { _offset += value; }

void symbol::value::Location::setOffset(quint64 value) { _offset = value; }

quint64 symbol::value::Location::offset() const { return _offset; }

quint64 symbol::value::Location::base() const { return _base; }

symbol::value::Pointer::Pointer() {}

symbol::value::Pointer::Pointer(QSharedPointer<const Entry> ptr)
    : symbol_pointer(ptr) {}

symbol::value::Pointer::Pointer(const Pointer &other)
    : symbol_pointer(other.symbol_pointer) {}

symbol::value::Pointer::Pointer(Pointer &&other) noexcept {
  swap(*this, other);
}

symbol::value::Pointer &symbol::value::Pointer::operator=(Pointer other) {
  swap(*this, other);
  return *this;
}

symbol::value::MaskedBits symbol::value::Pointer::value() const {
  if (symbol_pointer.isNull())
    return {.byteCount = 0, .bitPattern = 0, .mask = 0};
  return symbol_pointer->value->value();
}

symbol::Type symbol::value::Pointer::type() const {
  return symbol::Type::kPtrToSym;
}

QSharedPointer<symbol::value::Abstract> symbol::value::Pointer::clone() const {
  return QSharedPointer<Pointer>::create(*this);
}

quint64 symbol::value::MaskedBits::operator()() { return bitPattern & mask; }

bool symbol::value::MaskedBits::operator==(const MaskedBits &other) const {
  return this->byteCount == other.byteCount &&
         this->bitPattern == other.bitPattern && this->mask == other.mask;
}
