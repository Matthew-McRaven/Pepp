#pragma once
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast::generic {
struct PAS_EXPORT Location {
  qsizetype line = 0;
  bool valid = false;
  bool operator==(const Location &other) const = default;
};

struct PAS_EXPORT SourceLocation {
  static const inline QString attributeName = u"generic:source_loc"_qs;
  Location value = {}; // The location (line) line in the source file on which
                       // the node starts.
  bool operator==(const SourceLocation &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::SourceLocation);
