#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Hide {
  static const inline QString attributeName = u"generic:hide"_qs;
  struct In {
    bool source = false;
    bool listing = false;
    enum class Object {
      NoEmit_CountSize,
      NoEmit_NoCountSize,
      Emit
    } object = Object::Emit;
    bool addressInListing = false;

    bool operator==(const In &other) const = default;
  } value = {};
  bool operator==(const Hide &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Hide);
