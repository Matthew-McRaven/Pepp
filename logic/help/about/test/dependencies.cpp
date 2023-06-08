#include "about/dependencies.hpp"
#include <QTest>
#include <QtCore>

class About_Dependencies : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    auto deps = about::dependencies();
    QCOMPARE(deps.length(), 5);
    for (const auto &dep : deps)
      QCOMPARE_NE(dep.licenseText.size(), 0);
  }
};
#include "dependencies.moc"

QTEST_MAIN(About_Dependencies);
