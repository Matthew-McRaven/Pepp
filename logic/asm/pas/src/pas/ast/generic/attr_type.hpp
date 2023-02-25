#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Type {
  enum Types {
    Directive,
    Instruction,
    Comment,
    Blank,
  };
  static const inline QString attributeName = u"generic:type"_qs;
  Types value; // The type of the node (i.e., pseudodirective, comment)
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Type);
