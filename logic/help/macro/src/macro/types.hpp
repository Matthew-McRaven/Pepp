#pragma once

#include "macro_globals.hpp"
#include <QtCore>

namespace macro::types {
Q_NAMESPACE_EXPORT(MACRO_EXPORT)
enum Type : quint8 {
  Core,
  System,
  User,
};
Q_ENUM_NS(Type);
} // namespace macro::types
