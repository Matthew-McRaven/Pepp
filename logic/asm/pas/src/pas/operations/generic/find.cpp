#include "./find.hpp"
#include "pas/ast/node.hpp"

QSharedPointer<const pas::ast::Node> pas::ops::pepp::findFirst(const ast::Node &node,
                                                               SelectorFn selector)
{
    if(selector(node))
      return node.sharedFromThis();
    for(auto &child: ast::children(node))
      if(auto childFind = findFirst(*child, selector); childFind != nullptr) return childFind;
    return nullptr;
}

QSharedPointer<pas::ast::Node> pas::ops::pepp::findFirst(ast::Node &node,
                                                         SelectorFn selector)
{
    if(selector(node))
      return node.sharedFromThis();
    for(auto &child: ast::children(node))
      if(auto childFind = findFirst(*child, selector); childFind != nullptr) return childFind;
    return nullptr;
}
