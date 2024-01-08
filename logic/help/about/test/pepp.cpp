#include "help/about/pepp.hpp"
#include <QTest>
#include <QtCore>

class About_Pepp : public QObject {
  Q_OBJECT
private slots:
  void repoUrl() { QCOMPARE_NE(about::projectRepoURL().size(), 0); }
  void maintainers() { QCOMPARE_NE(about::maintainers().size(), 0); }
  void contributors() { QCOMPARE_NE(about::contributors().size(), 0); }
  void licenseText() { QCOMPARE_NE(about::licenseFull().size(), 0); }
  void licenseNotice() { QCOMPARE_NE(about::licenseNotice().size(), 0); }
  void version() { QCOMPARE_GT(about::versionString().size(), 1); }
};
#include "pepp.moc"

QTEST_MAIN(About_Pepp);
