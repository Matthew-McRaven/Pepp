#include "builtins/registry.hpp"
#include <QTest>
#include <QtCore>

class Builtins : public QObject {
  Q_OBJECT
private slots:
  void initTestCase() {}
  void createRegistry() { auto x = builtins::Registry(nullptr); }
};
// Must be after class declaration.
#include "dummy.test.moc"

QTEST_GUILESS_MAIN(Builtins);
