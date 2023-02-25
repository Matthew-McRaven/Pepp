#pragma once
#include <QtCore>

namespace pas::ast::value {
class Base;
}
namespace pas::ast::generic {
struct Argument {
  static const inline QString attributeName = u"generic:arg"_qs;
  QSharedPointer<value::Base> value = {};
};

struct ArgumentList {
  static const inline QString attributeName = u"generic:arg_list"_qs;
  QList<QSharedPointer<value::Base>> value = {};
};
} // namespace pas::ast::generic
Q_DECLARE_METATYPE(pas::ast::generic::Argument);
Q_DECLARE_METATYPE(pas::ast::generic::ArgumentList);
