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
#include <QtCore>
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/op.hpp"
#include "errors.hpp"
#include "is.hpp"

namespace pas::driver {
struct ParseResult;
}

namespace pas::ast {
class Node;
} // namespace pas::ast

namespace macro {
class Registry;
}

namespace pas::ops::generic {
using IsAddressedDirective = std::function<bool(ast::Node &)>;
struct IncludeMacros : public pas::ops::MutatingOp<bool> {
  struct MacroInvocation {
    QString macroName;
    QStringList args;
    bool operator==(const MacroInvocation &other) const = default;
    inline friend size_t qHash(const pas::ops::generic::IncludeMacros::MacroInvocation &invoke, size_t seed = 0) {
      seed = qHash(invoke.macroName, seed);
      for (const auto &arg : invoke.args) seed = qHash(arg, seed);
      return seed;
    }
  };
  QSharedPointer<macro::Registry> registry;
  // When we hit a directive inside a macro, function returns true if the directive has an address
  // e.g., BLOCK, BYTE. Must be an "argument" to this struct because different architectures allow different directives.
  IsAddressedDirective isDirectiveAddressed = [](auto &) { return false; };
  using node_t = QSharedPointer<pas::ast::Node>;
  std::function<driver::ParseResult(QString, node_t)> convertFn;
  bool operator()(ast::Node &node) override;
  bool pushMacroInvocation(MacroInvocation invoke);
  void popMacroInvocation(MacroInvocation invoke);
  void addExtraChildren(ast::Node &node);

private:
  QSet<MacroInvocation> _chain = {};
};

// BUG: Shouldn't be inline, but MSVC refuses to find this function in a CPP file, when GCC and clang can.
inline bool includeMacros(ast::Node &root,
                          std::function<pas::driver::ParseResult(QString, QSharedPointer<ast::Node>)> convertFn,
                          QSharedPointer<macro::Registry> registry, IsAddressedDirective isDirectiveAddressed) {
  static auto isMacro = pas::ops::generic::isMacro();
  auto converter = IncludeMacros();
  converter.convertFn = convertFn;
  converter.registry = registry;
  converter.isDirectiveAddressed = isDirectiveAddressed;
  ast::apply_recurse_if(root, isMacro, converter);
  auto errors = pas::ops::generic::CollectErrors();
  ast::apply_recurse(root, errors);
  return errors.errors.size() == 0;
}
} // namespace pas::ops::generic
