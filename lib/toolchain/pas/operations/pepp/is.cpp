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
#include "toolchain/pas/ast/generic/attr_sec.hpp"

#include <toolchain/pas/ast/generic/attr_directive.hpp>
bool isArgumentedDirective(const pas::ast::Node &node, const QString name) {
  using namespace pas;
  return node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>() &&
         (node.get<ast::generic::Directive>().value.toUpper() == name.toUpper()) && node.has<ast::generic::Argument>();
}
bool pas::ops::pepp::isBurn::operator()(const ast::Node &node) { return isArgumentedDirective(node, "BURN"); }

bool pas::ops::pepp::isEnd::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>() &&
         (node.get<ast::generic::Directive>().value.toUpper() == "END");
}

bool pas::ops::pepp::isExport::operator()(const ast::Node &node) { return isArgumentedDirective(node, "EXPORT"); }

bool pas::ops::pepp::isImport::operator()(const ast::Node &node) { return isArgumentedDirective(node, "IMPORT"); }

bool pas::ops::pepp::isInput::operator()(const ast::Node &node) { return isArgumentedDirective(node, "INPUT"); }

bool pas::ops::pepp::isOutput::operator()(const ast::Node &node) { return isArgumentedDirective(node, "OUTPUT"); }

bool pas::ops::pepp::isSCall::operator()(const ast::Node &node) { return isArgumentedDirective(node, "SCALL"); }

bool pas::ops::pepp::isSection::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>() &&
         (node.get<ast::generic::Directive>().value.toUpper() == "SECTION") &&
         (node.has<ast::generic::ArgumentList>() || node.has<ast::generic::Argument>());
}
