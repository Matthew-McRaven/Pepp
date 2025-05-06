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

#include "./whole_program_sanity.hpp"
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/value/symbolic.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/symbol/entry.hpp"

#include <toolchain/pas/ast/generic/attr_symbol.hpp>

bool pas::ops::pepp::IsOSFeature::operator()(const ast::Node &node) {
  static const auto osDirectives = QSet<QString>{"BURN", "EXPORT", "IMPORT", "INPUT", "OUTPUT", "SCALL", "USCALL"};
  return ast::type(node).value == ast::generic::Type::Directive &&
         osDirectives.contains(node.get<ast::generic::Directive>().value);
}

bool pas::ops::pepp::hasOSFeatures(ast::Node &node) {
  pas::ops::generic::Exists<IsOSFeature> feats;
  ast::apply_recurse(node, feats);
  return feats.result;
}

void pas::ops::pepp::ErrorOnOSFeatures::operator()(ast::Node &node) {
  IsOSFeature feat;
  if (feat(node)) {
    if (node.has<ast::generic::Directive>()) {
      auto name = node.get<ast::generic::Directive>().value;
      auto message = pas::errors::pepp::illegalInUser.arg("." + name);
      ast::addError(node,
                    ast::generic::Message{.severity = ast::generic::Message::Severity::Fatal, .message = message});
    } else {
      static const char *const e = "Unimplemented code path in ErrorsOnOSFeatures";
      qCritical(e);
      throw std::logic_error(e);
    }
  }
}

void pas::ops::pepp::errorOnOSFeatures(ast::Node &node) {
  ErrorOnOSFeatures visit;
  IsOSFeature is;
  ast::apply_recurse_if<void>(node, is, visit);
}

// Return true if there is an undefined symbolic argument.
// Also annotates the node with an appropriate error message.
bool annotateUndefinedArgument(pas::ast::Node &node, pas::ast::value::Base *arg) {
  auto casted = dynamic_cast<pas::ast::value::Symbolic *>(arg);
  if (casted == nullptr) return false;
  else if (casted->symbol()->state == symbol::DefinitionState::kUndefined) {
    pas::ast::addError(node, {.severity = pas::ast::generic::Message::Severity::Fatal,
                              .message = pas::errors::pepp::undefinedSymbol.arg(casted->symbol()->name)});
    return true;
  } else return false;
}

void pas::ops::pepp::ErrorOnUndefinedSymbolicArgument::operator()(ast::Node &node) {
  if (node.has<ast::generic::Argument>())
    hadError |= annotateUndefinedArgument(node, node.get<ast::generic::Argument>().value.data());
  else if (node.has<ast::generic::ArgumentList>())
    for (auto arg : node.get<ast::generic::ArgumentList>().value)
      hadError |= annotateUndefinedArgument(node, arg.data());
}

bool pas::ops::pepp::errorOnUndefinedSymbolicArgument(ast::Node &node) {
  ErrorOnUndefinedSymbolicArgument visit;
  ast::apply_recurse(node, visit);
  return visit.hadError;
}

void pas::ops::pepp::ErrorOnMultipleSymbolDefiniton::operator()(ast::Node &node) {
  if (node.has<ast::generic::SymbolDeclaration>()) {
    auto symbol = node.get<ast::generic::SymbolDeclaration>().value;
    // Don't need to check for undefined. Undefined is impossible if we have a
    // symbol declaration.
    if (symbol->state == symbol::DefinitionState::kSingle) return;
    hadError |= true;
    ast::addError(node, {.severity = pas::ast::generic::Message::Severity::Fatal,
                         .message = pas::errors::pepp::multiplyDefinedSymbol.arg(symbol->name)});
  }
}

bool pas::ops::pepp::errorOnMultipleSymbolDefiniton(ast::Node &node) {
  ErrorOnMultipleSymbolDefiniton visit;
  ast::apply_recurse(node, visit);
  return visit.hadError;
}
