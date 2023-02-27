#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Type {
  enum Types {
    Directive,
    Instruction,
    Comment,
    Blank,
    MacroInvoke,
    Structural, // Root or section group nodes
  };
  static const inline QString attributeName = u"generic:type"_qs;
  Types value; // The type of the node (i.e., pseudodirective, comment)
  bool operator==(const Type &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Type);
