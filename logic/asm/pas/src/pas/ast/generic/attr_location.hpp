#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct SourceLocation {
  static const inline QString attributeName = u"generic:source_loc"_qs;
  qsizetype value = 0; // The line in the source file on which the node starts.
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::SourceLocation);
