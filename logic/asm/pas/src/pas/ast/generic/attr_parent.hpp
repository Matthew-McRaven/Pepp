#pragma once
#include <QtCore>
namespace pas::ast {
class Node;
}
namespace pas::ast::generic {
struct Parent {
  static const inline QString attributeName = u"generic:parent"_qs;
  QWeakPointer<pas::ast::Node> value =
      {}; // The direct parent of the owning node (empty if owner is root).
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Parent);
