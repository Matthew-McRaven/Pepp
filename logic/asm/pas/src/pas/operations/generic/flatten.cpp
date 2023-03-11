#include "flatten.hpp"
#include "is.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/ast/node.hpp"

void pas::ops::generic::flattenMacros(ast::Node &node) {
  if (node.has<ast::generic::Children>()) {
    auto newChildren = QList<QSharedPointer<ast::Node>>{};
    for (auto &child : node.take<ast::generic::Children>().value) {
      flattenMacros(*child);
      if (isMacro()(*child)) {
        for (auto &macroChild : child->get<ast::generic::Children>().value)
          newChildren.append(macroChild);
      } else
        newChildren.append(child);
    }

    node.set(ast::generic::Children{.value = newChildren});
  }
  return;
}
