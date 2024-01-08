#pragma once
#include <QtCore>

namespace about {
struct Dependency {
  QString name, url, licenseName, licenseSPDXID, licenseText;
  bool devDependency;
};
QList<Dependency> dependencies();
} // namespace about
