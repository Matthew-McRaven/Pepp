#include <QObject>
#include <QTest>

class PasOpsPepp_AssignAddress : public QObject {
  Q_OBJECT
private slots:
  void smoke() {}
};

#include "assign_addr.test.moc"

QTEST_MAIN(PasOpsPepp_AssignAddress)
