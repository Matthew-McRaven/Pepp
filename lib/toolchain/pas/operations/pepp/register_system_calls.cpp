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

#include "./register_system_calls.hpp"
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/pas/operations/pepp/is.hpp"
#include "toolchain/macro/macro.hpp"

#include <toolchain/pas/ast/value/symbolic.hpp>

// TODO: Determine if 1-indexed of 0-indexed.
// Must manually add %1, %2 later. Macro syntax conflicts with QString::arg, and
// can't escape %#.
const QString nonunarySCallMacro = "LDWA %1, i\nSCALL ";
using pas::ast::generic::Message;

bool pas::ops::pepp::RegisterSystemCalls::operator()(ast::Node &node) {
  using namespace Qt::StringLiterals;
  auto macroKind = node.get<ast::generic::Directive>().value;
  QSharedPointer<macro::Parsed> parsed = {};

  // Validate that directive has correct argument types, then construct correct
  // macro kind (unary/non-unary)
  if (!node.has<ast::generic::Argument>()) {
    addedError = true;
    ast::addError(node, {.severity = Message::Severity::Fatal, .message = u"%1 missing argument."_s.arg(macroKind)});
  } else if (auto argument = dynamic_cast<ast::value::Symbolic *>(&*node.get<ast::generic::Argument>().value);
             argument == nullptr) {
    addedError = true;
    ast::addError(node, {.severity = Message::Severity::Fatal,
                         .message = u"%1 expected a identifier argument."_s.arg(macroKind)});
  } else if (macroKind.toUpper() == "SCALL") {
    auto name = argument->string();
    parsed = QSharedPointer<macro::Parsed>::create(name, 2, nonunarySCallMacro.arg(name) + "$1, $2\n", "pep/10");
  } else {
    addedError = true;
    ast::addError(node, {.severity = Message::Severity::Fatal, .message = u"Unspecified error."_s});
  }

  // Attempt to register macro, and propogate error if it already exists.
  if (!parsed.isNull() && registry->registerMacro(macro::types::System, parsed) == nullptr) {
    addedError = true;
    ast::addError(node, {.severity = Message::Severity::Fatal, .message = u"Duplicate system call."_s});
  }
  return addedError;
}

bool pas::ops::pepp::registerSystemCalls(ast::Node &node, QSharedPointer<macro::Registry> registry) {
  auto is = pas::ops::pepp::isSCall();
  auto visit = RegisterSystemCalls();
  visit.registry = registry;
  ast::apply_recurse_if(node, is, visit);
  return !visit.addedError;
}
