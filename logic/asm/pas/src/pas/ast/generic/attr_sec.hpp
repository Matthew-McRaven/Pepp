#pragma once
#include <QtCore>

namespace pas::ast::generic {

struct SectionFlags {
  static const inline QString attributeName = u"generic:section_flags"_qs;
  struct Flags {
    bool R = 1, W = 1, X = 1, Z = 0;
    bool operator==(const Flags &other) const = default;
  } value = {};
  bool operator==(const SectionFlags &other) const = default;
};
struct SectionName {
  static const inline QString attributeName = u"generic:section_name"_qs;
  QString value = {};
  bool operator==(const SectionName &other) const = default;
};

} // namespace pas::ast::generic
Q_DECLARE_METATYPE(pas::ast::generic::SectionFlags);
Q_DECLARE_METATYPE(pas::ast::generic::SectionName);
