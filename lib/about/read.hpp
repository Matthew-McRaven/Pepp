#pragma once
#include <QtCore>
#include <optional>
namespace about::detail {
std::optional<QString> readFile(QString fname);
class ReadHelper : public QObject {
  Q_OBJECT
public:
  ReadHelper(QObject *parent = nullptr);
  Q_INVOKABLE QString readFile(QString fname);
};
}
