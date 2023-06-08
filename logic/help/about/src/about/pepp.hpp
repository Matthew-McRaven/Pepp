#pragma once
#include <QtCore>

namespace about {
QString projectRepoURL();
struct Maintainer {
  QString name, email;
};
QList<Maintainer> maintainers();
QList<QString> contributors();
QString licenseFull();
QString licenseNotice();
QString versionString();
} // namespace about
