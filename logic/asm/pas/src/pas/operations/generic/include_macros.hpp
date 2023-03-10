#pragma once
#include "pas/ast/op.hpp"
#include <QtCore>

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
    inline friend size_t
    qHash(const pas::ops::generic::IncludeMacros::MacroInvocation &invoke,
          size_t seed = 0) {
      seed = qHash(invoke.macroName, seed);
      for (const auto &arg : invoke.args)
        seed = qHash(arg, seed);
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

bool includeMacros(
    ast::Node &root,
    std::function<driver::ParseResult(QString, QSharedPointer<ast::Node>)>
        convert,
    QSharedPointer<macro::Registry>);
} // namespace pas::ops::generic
