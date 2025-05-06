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

#include "./is.hpp"
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/generic/attr_comment.hpp"
#include "toolchain/pas/ast/generic/attr_comment_indent.hpp"
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/ast/generic/attr_macro.hpp"
#include "toolchain/pas/ast/node.hpp"

#include <toolchain/pas/ast/generic/attr_hide.hpp>

bool pas::ops::generic::isBlank::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Blank;
}

bool pas::ops::generic::isComment::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Comment && node.has<ast::generic::Comment>() &&
         node.has<ast::generic::CommentIndent>();
}

bool pas::ops::generic::isAlign::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>() &&
         node.get<ast::generic::Directive>().value.toUpper() == "ALIGN" && node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isString::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>() &&
         directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper()) &&
         node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isSkip::operator()(const ast::Node &node) {
  if (allowFill)
    return node.get<ast::generic::Type>().value == ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::ArgumentList>();
  else
    return node.get<ast::generic::Type>().value == ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isByte1::operator()(const ast::Node &node) {
  if (allowMultiple)
    return node.get<ast::generic::Type>().value == ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::ArgumentList>();
  else
    return node.get<ast::generic::Type>().value == ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isByte2::operator()(const ast::Node &node) {
  if (allowMultiple)
    return node.get<ast::generic::Type>().value == ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::ArgumentList>();
  else
    return node.get<ast::generic::Type>().value == ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isSet::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>() &&
         directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper()) &&
         node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isDirective::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>();
}

bool pas::ops::generic::isMacro::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::MacroInvoke && node.has<ast::generic::Macro>();
}

bool pas::ops::generic::isStructural::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Structural;
}

bool pas::ops::generic::SourceHidden::operator()(const ast::Node &node) {
  return node.has<ast::generic::Hide>() && node.get<ast::generic::Hide>().value.source;
}

bool pas::ops::generic::ListingHidden::operator()(const ast::Node &node) {
  return node.has<ast::generic::Hide>() && node.get<ast::generic::Hide>().value.listing;
}

bool pas::ops::generic::isOrg::operator()(const ast::Node &node) {
  using namespace pas;
  return node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>() &&
         (directiveAliases.contains(node.get<ast::generic::Directive>().value.toUpper())) &&
         node.has<ast::generic::Argument>();
}
