#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Macro {
  static const inline QString attributeName = u"generic:macro"_qs;
  QString value = {};
  bool operator==(const Macro &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Macro);
