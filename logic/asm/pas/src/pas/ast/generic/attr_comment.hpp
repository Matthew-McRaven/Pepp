#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Comment {
  static const inline QString attributeName = u"generic:comment"_qs;
  QString value = {};
  bool operator==(const Comment &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Comment);
