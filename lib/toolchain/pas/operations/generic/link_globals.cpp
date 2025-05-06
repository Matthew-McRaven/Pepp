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

#include "link_globals.hpp"
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/value/symbolic.hpp"
#include "toolchain/pas/driver/common.hpp"
#include "toolchain/symbol/entry.hpp"
#include "toolchain/symbol/table.hpp"
#include "toolchain/symbol/visit.hpp"

void pas::ops::generic::LinkGlobals::operator()(ast::Node &node) {
  if (node.has<ast::generic::SymbolDeclaration>()) {
    auto local = node.get<ast::generic::SymbolDeclaration>().value;
    updateSymbol(local);
  }
  if (node.has<ast::generic::Directive>() &&
      exportDirectives.contains(node.get<ast::generic::Directive>().value.toUpper())) {
    // Encountered global declaration. Register symbol as global within own
    // hierarchy, and register with globals table.
    if (node.has<ast::generic::Argument>()) {
      auto arg = node.get<ast::generic::Argument>().value;
      if (auto casted = dynamic_cast<ast::value::Symbolic *>(&*arg); casted != nullptr) {
        auto symbol = casted->symbol();
        globals->add(symbol);
        symbol->parent.markGlobal(symbol->name);
      }
    }
  }
  // Check if argument(s) use global symbols.
  else if (node.has<ast::generic::Argument>()) {
    auto arg = node.get<ast::generic::Argument>().value;
    if (auto casted = dynamic_cast<ast::value::Symbolic *>(&*arg); casted != nullptr) updateSymbol(casted->symbol());
  } else if (node.has<ast::generic::ArgumentList>()) {
    auto args = node.get<ast::generic::ArgumentList>().value;
    for (auto &arg : args)
      if (auto casted = dynamic_cast<ast::value::Symbolic *>(&*arg); casted != nullptr) updateSymbol(casted->symbol());
  }
}

void pas::ops::generic::LinkGlobals::updateSymbol(QSharedPointer<symbol::Entry> symbol) {
  if (!globals->contains(symbol->name)) return;
  QSharedPointer<symbol::Entry> global = globals->get(symbol->name);
  // If the symbols belong to the same hierarchy, then the logic for exports
  // will bind the symbols together.
  if (symbol::rootTable(global->parent.sharedFromThis()) == symbol::rootTable(symbol->parent.sharedFromThis())) return;
  symbol->binding = symbol::Binding::kImported;
  switch (symbol->state) {
  case symbol::DefinitionState::kUndefined: symbol->state = symbol::DefinitionState::kSingle; break;
  case symbol::DefinitionState::kSingle: // A symbol that is already defined
                                         // conflicts with global declaration
    [[fallthrough]];
  // Multiply define symbols remain multiply defined.
  case symbol::DefinitionState::kMultiple: [[fallthrough]];
  case symbol::DefinitionState::kExternalMultiple: symbol->state = symbol::DefinitionState::kExternalMultiple; break;
  }
  symbol->value = QSharedPointer<symbol::value::ExternalPointer>::create(symbol->parent.pointerSize(),
                                                                         global->parent.sharedFromThis(), global);
}

void pas::ops::generic::linkGlobals(ast::Node &node, QSharedPointer<driver::Globals> globals,
                                    QSet<QString> exportDirectives) {
  auto visit = LinkGlobals();
  visit.globals = globals;
  visit.exportDirectives = exportDirectives;
  ast::apply_recurse(node, visit);
}
