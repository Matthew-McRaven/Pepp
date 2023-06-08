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
#include "pas/parse/pepp/types_values.hpp"
#include "symbol/table.hpp"
#include <QtCore>
#include <boost/variant/static_visitor.hpp>
#include "pas/pas_globals.hpp"

namespace pas::ast::value {
class Base;
};

namespace pas::parse::pepp {
struct PAS_EXPORT ParseToArg
    : public boost::static_visitor<QSharedPointer<pas::ast::value::Base>> {
  bool preferIdent = false;             // in
  QSharedPointer<symbol::Table> symTab; // in
  std::optional<quint8> sizeOverride = std::nullopt;
  QSharedPointer<pas::ast::value::Base> operator()(const CharacterLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const StringLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const Identifier &);
  QSharedPointer<pas::ast::value::Base> operator()(const DecimalLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const HexadecimalLiteral &);
};
} // namespace pas::parse::pepp
