#include "./read.hpp"

std::optional<QString> about::detail::readFile(QString fname) {
  QFile file(fname);
  QString fileText;
  if (file.open(QFile::ReadOnly)) fileText = file.readAll();
  else {
    qWarning() << "Failed to open: " << fname << ".\n";
    return std::nullopt;
  }
  return fileText;
}

about::detail::ReadHelper::ReadHelper(QObject *parent) : QObject(parent) {}

QString about::detail::ReadHelper::readFile(QString fname) {
  auto ret = about::detail::readFile(fname);
  return ret.value_or("");
}
