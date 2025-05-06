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

#include "./symbolic.hpp"
#include "toolchain/symbol/entry.hpp"
#include "utils/bits/copy.hpp"
#include "utils/bits/order.hpp"
pas::ast::value::Symbolic::Symbolic() {}

pas::ast::value::Symbolic::Symbolic(QSharedPointer<symbol::Entry> value) : Base(), _value(value) {}

pas::ast::value::Symbolic::Symbolic(const Symbolic &other) : Base() {}

pas::ast::value::Symbolic::Symbolic(Symbolic &&other) noexcept : Symbolic() { swap(*this, other); }

pas::ast::value::Symbolic &pas::ast::value::Symbolic::operator=(Symbolic other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<symbol::Entry> pas::ast::value::Symbolic::symbol() { return _value; }

QSharedPointer<const symbol::Entry> pas::ast::value::Symbolic::symbol() const { return _value; }

QSharedPointer<pas::ast::value::Base> pas::ast::value::Symbolic::clone() const {
  // BUG: Symbol needs to be forked into a new table!!
  return QSharedPointer<Symbolic>::create(*this);
}

void pas::ast::value::Symbolic::value(bits::span<quint8> dest, bits::Order destEndian) const {
  auto src = _value->value->value()();
  using size_type = bits::span<const quint8>::size_type;
  auto srcSpan = bits::span<const quint8>{reinterpret_cast<const quint8 *>(&src), static_cast<size_type>(size())};
  bits::memcpy_endian(dest, destEndian, srcSpan, bits::hostOrder());
}

quint64 pas::ast::value::Symbolic::size() const { return _value->value->value().byteCount; }

bool pas::ast::value::Symbolic::resize(quint64 size) { return false; }

quint64 pas::ast::value::Symbolic::requiredBytes() const { return ceil(log2(_value->value->value()() + 1) / 8); }

QString pas::ast::value::Symbolic::string() const { return _value->name; }

QString pas::ast::value::Symbolic::rawString() const { return string(); }
