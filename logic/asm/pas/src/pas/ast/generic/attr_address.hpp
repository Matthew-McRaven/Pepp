#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Address {
  struct Span {
    quint64 start, end;
    bool operator==(const Span &other) const = default;
  };

  static const inline QString attributeName = u"generic:address"_qs;
  QString value = {};
  bool operator==(const Address &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Address);
