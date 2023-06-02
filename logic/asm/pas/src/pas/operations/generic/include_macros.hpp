#pragma once
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "errors.hpp"
#include "is.hpp"
#include <QtCore>
#include "pas/pas_globals.hpp"

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
struct PAS_EXPORT IncludeMacros : public pas::ops::MutatingOp<bool> {
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

// BUG: Shouldn't be inline, but MSVC refuses to find this function in a CPP file, when GCC and clang can.
inline bool includeMacros(
    ast::Node &root,
    std::function<pas::driver::ParseResult(QString, QSharedPointer<ast::Node>)>
        convertFn,
    QSharedPointer<macro::Registry> registry)
{
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
