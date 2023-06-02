#pragma once
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast::value {
class Base;
}
namespace pas::ast::generic {
struct PAS_EXPORT Argument {
  static const inline QString attributeName = u"generic:arg"_qs;
  QSharedPointer<value::Base> value = {};
  bool operator==(const Argument &other) const = default;
};

struct PAS_EXPORT ArgumentList {
  static const inline QString attributeName = u"generic:arg_list"_qs;
  QList<QSharedPointer<value::Base>> value = {};
  bool operator==(const ArgumentList &other) const = default;
};
} // namespace pas::ast::generic
Q_DECLARE_METATYPE(pas::ast::generic::Argument);
Q_DECLARE_METATYPE(pas::ast::generic::ArgumentList);
