#pragma once
#include <QtCore>
#include <optional>
namespace about::detail {
  std::optional<QString> readFile(QString fname);
}
