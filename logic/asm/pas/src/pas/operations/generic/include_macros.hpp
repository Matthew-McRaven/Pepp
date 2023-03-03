#pragma once
#include "pas/ast/op.hpp"
#include <QtCore>
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
    inline friend size_t qHash(const pas::ops::generic::IncludeMacros::MacroInvocation& invoke, size_t seed=0) {
        seed = qHash(invoke.macroName, seed);
        for(const auto& arg : invoke.args)
            seed = qHash(arg, seed);
        return seed;
    }
  };
  QSharedPointer<macro::Registry> registry;
  using node_t = QSharedPointer<pas::ast::Node>;
  std::function<QList<node_t>(QString)> convertFn;
  bool operator()(ast::Node &node) override;
  bool pushMacroInvocation(MacroInvocation invoke);
  void popMacroInvocation(MacroInvocation invoke);

private:
  QSet<MacroInvocation> _chain = {};
};

bool includeMacros(
    ast::Node &root,
    std::function<QList<QSharedPointer<pas::ast::Node>>(QString)> convert,
    QSharedPointer<macro::Registry>);
} // namespace pas::ops::generic
