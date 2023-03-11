#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Directive {
  static const inline QString attributeName = u"generic:directive"_qs;
  QString value = {};
  bool operator==(const Directive &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Directive);
