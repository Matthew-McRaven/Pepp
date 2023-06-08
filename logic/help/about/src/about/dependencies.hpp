#pragma once
#include <QtCore>

namespace about {
struct Dependency {
  QString name, url, licenseName, licenseSPDXID, licenseText;
};
QList<Dependency> dependencies();
} // namespace about
