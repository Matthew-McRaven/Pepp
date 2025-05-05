#pragma once
#include <QtCore>
#include <QtQmlIntegration>
#include <optional>

namespace about::detail {
std::optional<QString> readFile(QString fname);
class ReadHelper : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(FileReader)
  QML_SINGLETON

public:
  ReadHelper(QObject *parent = nullptr);
  Q_INVOKABLE QString readFile(QString fname);
};
} // namespace about::detail
