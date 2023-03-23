#include <QTest>
#include <QObject>

class PasOpsPepp_ToBytes : public QObject {
    Q_OBJECT
private slots:
    void smoke(){}
};

#include "to_bytes.test.moc"

QTEST_MAIN(PasOpsPepp_ToBytes)
