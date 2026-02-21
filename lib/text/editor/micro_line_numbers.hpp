#pragma once

#include <QObject>
#include <QtQmlIntegration>
#include "core/ds/linenumbers.hpp"
namespace pepp {
class LineNumbers : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("Only createable in C++")
public:
  explicit LineNumbers(pepp::Line2Address l2a, QObject *parent = nullptr);
  pepp::Line2Address l2a;
};
} // namespace pepp
