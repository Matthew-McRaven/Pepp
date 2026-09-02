#pragma once
#include <QByteArray>
#include <QFile>
#include <QString>
#include <QtCore/qdebug.h>
#include <cstdint>
#include <string>
#include <vector>

inline std::vector<uint8_t> load(const std::string &filename) {
  auto as_qs = QString::fromStdString(filename);

  QFile file(as_qs);
  QByteArray ret;
  if (file.size() > 1'000'000) {
    qFatal() << "File size exceeds 1MB. Will not load.";
  } else if (file.open(QIODevice::ReadOnly)) {
    ret = file.readAll();
    file.close();
  } else qFatal() << "Could not open file for reading";
  return std::vector<uint8_t>(ret.begin(), ret.end());
}
