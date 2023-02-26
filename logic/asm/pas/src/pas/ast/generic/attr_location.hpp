#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Location {
  qsizetype line = 0;
  bool valid = false;
  bool operator==(const Location &other) const = default;
};

struct SourceLocation {
  static const inline QString attributeName = u"generic:source_loc"_qs;
  Location value = {}; // The location (line) line in the source file on which
                       // the node starts.
  bool operator==(const SourceLocation &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::SourceLocation);
