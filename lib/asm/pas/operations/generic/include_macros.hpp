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
#include "asm/pas/ast/node.hpp"
#include "asm/pas/ast/op.hpp"
#include "errors.hpp"
#include "is.hpp"

namespace pas::driver {
class ParseResult;
}

namespace pas::ast {
class Node;
} // namespace pas::ast

namespace macro {
class Registry;
}

namespace pas::ops::generic {
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
                          QSharedPointer<macro::Registry> registry) {
  static auto isMacro = pas::ops::generic::isMacro();
  auto converter = IncludeMacros();
  converter.convertFn = convertFn;
  converter.registry = registry;
  ast::apply_recurse_if(root, isMacro, converter);
  auto errors = pas::ops::generic::CollectErrors();
  ast::apply_recurse(root, errors);
  return errors.errors.size() == 0;
}
} // namespace pas::ops::generic
