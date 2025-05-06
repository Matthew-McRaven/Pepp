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

#include "./clone.hpp"
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/ast/generic/attr_parent.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/generic/attr_type.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/value/symbolic.hpp"
#include "toolchain/symbol/fork.hpp"
#include "toolchain/symbol/table.hpp"

QSharedPointer<pas::ast::Node> pas::ops::generic::clone::operator()(const ast::Node &node) {
  using namespace ast::generic;
  auto cloned = QSharedPointer<ast::Node>::create(node.get<Type>());
  auto attr = cloned->attributes();
  cloned->fromAttributes(attr);
  for (auto key = attr.keyBegin(); key != attr.keyEnd(); ++key) {
    if (*key == ast::generic::Argument::attribute) {
      auto symbolArg = dynamic_cast<ast::value::Symbolic *>(node.get<Argument>().value.data());
      if (symbolArg != nullptr) {
        auto argument = QSharedPointer<ast::value::Symbolic>::create(entry(&*symbolArg->symbol()));
        cloned->set(Argument{.value = argument});
      } else cloned->set(node.get<Argument>());
    } else if (*key == ast::generic::ArgumentList::attribute) {
      auto args = node.get<ast::generic::ArgumentList>().value;
      for (int it = 0; it < args.size(); it++) {
        auto symbolArg = dynamic_cast<ast::value::Symbolic *>(args[it].data());
        if (symbolArg != nullptr) args[it] = QSharedPointer<ast::value::Symbolic>::create(entry(&*symbolArg->symbol()));
      }
      cloned->set(ArgumentList{.value = args});
    } else if (*key == ast::generic::SymbolTable::attribute) {
      cloned->set(SymbolTable{.value = table(&*cloned->get<ast::generic::SymbolTable>().value)});
    } else if (*key == ast::generic::SymbolDeclaration::attribute) {
      auto oldSymbolDec = node.get<SymbolDeclaration>().value;
      // Value replication is handled by symbol table `fork`.
      auto newSymbolDec = SymbolDeclaration{.value = entry(&*oldSymbolDec)};
      cloned->set<SymbolDeclaration>(newSymbolDec);
    } else if (*key == ast::generic::Parent::attribute) {
      cloned->set(Parent{.value = {}});
    } else if (*key == ast::generic::Children::attribute) {
      cloned->set(Children{.value = {}});
    }
  }
  return cloned;
}
QSharedPointer<symbol::Entry> pas::ops::generic::clone::entry(const symbol::Entry *entry) {
  return mapping->map(entry);
}

QSharedPointer<symbol::Table> pas::ops::generic::clone::table(const symbol::Table *table) {
  return mapping->map(table);
}
