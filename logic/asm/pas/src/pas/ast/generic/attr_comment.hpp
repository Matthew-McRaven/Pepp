#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Comment {
  static const inline QString attributeName = u"generic:comment"_qs;
  QString value =
      {};
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Comment);
