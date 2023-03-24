#include <QTest>
#include <QObject>

class PasOpsPepp_FormatListing : public QObject {
  Q_OBJECT
private slots:
  void smoke(){}
};

#include "listing.test.moc"

QTEST_MAIN(PasOpsPepp_FormatListing)
