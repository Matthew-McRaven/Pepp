#pragma once
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast::generic {
struct PAS_EXPORT CommentIndent {
  enum class Level { Left, Instruction };
  static const inline QString attributeName = u"generic:comment_indent"_qs;
  Level value = Level::Left;
  bool operator==(const CommentIndent &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::CommentIndent);
