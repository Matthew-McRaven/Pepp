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

#include "./identifier.hpp"

pas::ast::value::Identifier::Identifier() : Base() {}

pas::ast::value::Identifier::Identifier(QString value) : _value(value) {}

pas::ast::value::Identifier::Identifier(const Identifier &other) : Base(), _value(other._value) {}

pas::ast::value::Identifier::Identifier(Identifier &&other) noexcept { swap(*this, other); }

pas::ast::value::Identifier &pas::ast::value::Identifier::operator=(Identifier other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base> pas::ast::value::Identifier::clone() const {
  return QSharedPointer<Identifier>::create(*this);
}

void pas::ast::value::Identifier::value(bits::span<quint8> dest, bits::Order destEndian) const {}

quint64 pas::ast::value::Identifier::size() const { return 0; }

bool pas::ast::value::Identifier::resize(quint64 size) { return false; }

quint64 pas::ast::value::Identifier::requiredBytes() const { return 0; }

QString pas::ast::value::Identifier::string() const { return _value; }

QString pas::ast::value::Identifier::rawString() const { return string(); }
