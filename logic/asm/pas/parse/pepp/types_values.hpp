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
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <climits>
#include <string>
#include "asm/pas/pas_globals.hpp"

namespace pas::parse::pepp {
struct PAS_EXPORT TextLiteral {
  std::string value;
};
struct PAS_EXPORT CharacterLiteral : public TextLiteral {};

struct PAS_EXPORT StringLiteral : public TextLiteral {};
struct PAS_EXPORT Identifier : public TextLiteral {};

struct PAS_EXPORT NumericLiteral {
  uint64_t value;
};
struct PAS_EXPORT DecimalLiteral : public NumericLiteral {
  bool isSigned = false;
};
struct PAS_EXPORT HexadecimalLiteral : public NumericLiteral {};

typedef boost::variant<StringLiteral, CharacterLiteral, Identifier,
                       DecimalLiteral, HexadecimalLiteral>
    Value;
using boost::fusion::operator<<;
} // namespace pas::parse::pepp

BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::CharacterLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::StringLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::Identifier, value);

BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::DecimalLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::HexadecimalLiteral, value);
