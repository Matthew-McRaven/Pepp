#include <QObject>
#include <QTest>

class PasOpsPepp_AssignAddress : public QObject {
  Q_OBJECT
private slots:
  void unary() {}
  void nonUnary() {}
  void size0Directives() {}
  void size0Directives_data() {}
  void ascii() {}
  void ascii_data() {}
  void align() {}
  void align_data() {}
  void macro() {}
  void empty() {}
};

#include "assign_addr.test.moc"

QTEST_MAIN(PasOpsPepp_AssignAddress)
