#pragma once

#include <QtCore>
#include "macro_globals.hpp"

namespace macro {
  Q_NAMESPACE_EXPORT(MACRO_EXPORT)
  enum class Type : quint8 {
    Core,
    System,
    User,
  };
  Q_ENUM_NS(Type);
}
