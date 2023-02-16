#pragma once

#include <QtCore>
namespace macro {
  Q_NAMESPACE
  enum class Type : quint8 {
    Core,
    System,
    User,
  };
  Q_ENUM_NS(Type);
}
