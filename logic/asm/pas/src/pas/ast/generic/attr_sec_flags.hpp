#pragma once
#include <QtCore>

namespace pas::ast::generic {

struct SectionFlags {
  static const inline QString attributeName = u"generic:section_flags"_qs;
  struct Flags {
    bool R =1, W=1, X=1;
  } value = {};
};

} // namespace pas::ast::generic
