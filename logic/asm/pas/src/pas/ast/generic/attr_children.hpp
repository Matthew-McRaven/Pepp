#pragma once
#include <QtCore>
namespace pas::ast {
class Node;
}
namespace pas::ast::generic {
struct Children {
  static const inline QString attributeName = u"generic:children"_qs;
  QList<QSharedPointer<pas::ast::Node>> value =
      {}; // All direct children of the owning node
  // TODO: replace with element-wise value comparison.
  bool operator==(const Children &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Children);
