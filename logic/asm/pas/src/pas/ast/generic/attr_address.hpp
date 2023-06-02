#pragma once
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast::generic {
struct PAS_EXPORT Address {
  struct Span {
    quint64 start = 0, size = 0;
    bool operator==(const Span &other) const = default;
  };

  static const inline QString attributeName = u"generic:address"_qs;
  Span value = {};
  bool operator==(const Address &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Address);
