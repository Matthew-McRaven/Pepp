#include <QObject>
#include <QTest>

class PasOpsPepp_WholeProgramSanity : public QObject {
    Q_OBJECT
private slots:
    void smoke(){}
};

#include "whole_program_sanity.test.moc"

QTEST_MAIN(PasOpsPepp_WholeProgramSanity)
