#include "./assign_addr.hpp"
#include "is.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/value/base.hpp"

bool pas::ops::pepp::detail::hasBurn(
    QList<QSharedPointer<pas::ast::Node>> &list) {
  for (auto &child : list)
    if (pas::ops::pepp::isBurn()(*child))
      return true;
  return false;
}

quint16 pas::ops::pepp::detail::getBurnArg(
    QList<QSharedPointer<pas::ast::Node>> &list) {
  quint16 ret = 0xFFFF;
  for (auto &child : list)
    if (pas::ops::pepp::isBurn()(*child)) {
      if (!child->has<pas::ast::generic::Argument>())
        return ret;
      else {
        child->get<pas::ast::generic::Argument>().value->value(
            reinterpret_cast<quint8 *>(&ret), 2);
        return ret;
      }
    }
  return ret;
}
