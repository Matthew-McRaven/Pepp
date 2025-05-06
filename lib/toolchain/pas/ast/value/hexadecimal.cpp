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

#include "hexadecimal.hpp"

pas::ast::value::Hexadecimal::Hexadecimal() : Numeric() {}

pas::ast::value::Hexadecimal::Hexadecimal(quint64 value, quint16 size) : Numeric(value, size) {}

pas::ast::value::Hexadecimal::Hexadecimal(const Hexadecimal &other) : Numeric(other) {}

pas::ast::value::Hexadecimal::Hexadecimal(Hexadecimal &&other) noexcept { swap(*this, other); }

pas::ast::value::Hexadecimal &pas::ast::value::Hexadecimal::operator=(Hexadecimal other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base> pas::ast::value::Hexadecimal::clone() const {
  return QSharedPointer<Hexadecimal>::create(*this);
}

QString pas::ast::value::Hexadecimal::string() const {
  using namespace Qt::StringLiterals;
  return u"0x%1"_s.arg(QString::number(_value, 16).toUpper(), 2 * _size, QChar('0'));
}

QString pas::ast::value::Hexadecimal::rawString() const { return string(); }
