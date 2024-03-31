#pragma once

#define SHARED_CONSTANT(type, name, value)                                                                             \
  static inline const type name = value;                                                                               \
  Q_PROPERTY(type name MEMBER name CONSTANT)

namespace constants {
static const char *error_only_enums = "Only contains enums";
}
