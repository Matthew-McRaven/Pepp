#include "dummy.hpp"

#include <QDebug>
#include <QFile>
#include <QDirIterator>

void dummy(){
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
}
