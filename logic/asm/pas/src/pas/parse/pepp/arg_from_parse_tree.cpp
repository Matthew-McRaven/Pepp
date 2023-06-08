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

#include "./arg_from_parse_tree.hpp"
#include "bits/strings.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/ast/value/character.hpp"
#include "pas/ast/value/decimal.hpp"
#include "pas/ast/value/hexadecimal.hpp"
#include "pas/ast/value/identifier.hpp"
#include "pas/ast/value/string.hpp"
#include "pas/ast/value/symbolic.hpp"
using namespace pas::ast::value;
QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const CharacterLiteral &line) {
  return QSharedPointer<pas::ast::value::Character>::create(
      QString::fromStdString(line.value));
}

QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const StringLiteral &line) {
  auto asQString = QString::fromStdString(line.value);
  if (auto length = bits::escapedStringLength(asQString); length <= 2) {
    return QSharedPointer<pas::ast::value::ShortString>::create(
        asQString, length, bits::Order::BigEndian);
  } else
    return QSharedPointer<pas::ast::value::LongString>::create(
        asQString, bits::Order::BigEndian);
}

QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const Identifier &line) {
  auto asQString = QString::fromStdString(line.value);
  if (preferIdent)
    return QSharedPointer<pas::ast::value::Identifier>::create(asQString);
  else {
    auto asSymbol = symTab->reference(asQString);
    return QSharedPointer<pas::ast::value::Symbolic>::create(asSymbol);
  }
}

QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const DecimalLiteral &line) {
  if (line.isSigned)
    return QSharedPointer<pas::ast::value::SignedDecimal>::create(
        line.value, sizeOverride.value_or(2));
  else
    return QSharedPointer<pas::ast::value::UnsignedDecimal>::create(
        line.value, sizeOverride.value_or(2));
}

QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const HexadecimalLiteral &line) {
  return QSharedPointer<pas::ast::value::Hexadecimal>::create(
      line.value, sizeOverride.value_or(2));
}
