#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QSet>
#include "../memory_globals.hpp"

class MEMORY_EXPORT MemoryColumns : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint8 LineNo READ LineNo CONSTANT)
    Q_PROPERTY(quint8 Border1 READ Border1 CONSTANT)
    Q_PROPERTY(quint8 Border2 READ Border2 CONSTANT)
    Q_PROPERTY(quint8 CellStart READ CellStart CONSTANT)
    Q_PROPERTY(quint8 CellEnd READ CellEnd CONSTANT)
    Q_PROPERTY(quint8 Ascii READ Ascii CONSTANT)

    const quint8 lineNo_ = 0;
    const quint8 border1_ = 1;
    const quint8 cellStart_ = 2;
    quint8 cellEnd_ = cellStart_ + 7;
    quint8 border2_ = cellEnd_ + 1;
    quint8 ascii_ = border2_ + 1;
    quint8 total_ = ascii_ + 1;

public: //  Required by QML
    //  QML Properties
    quint8 LineNo() const { return lineNo_; };
    quint8 Border1() const { return border1_; };
    quint8 Border2() const { return border2_; };
    quint8 CellStart() const { return cellStart_; };
    quint8 CellEnd() const { return cellEnd_; };
    quint8 Ascii() const { return ascii_; };
    quint8 Total() const { return total_; };

    //  If model changes width, adjust column numbers
    void setNumBytesPerLine(const quint8 bytesPerLine)
    {
        cellEnd_ = cellStart_ + bytesPerLine - 1;
        border2_ = cellEnd_ + 1;
        ascii_ = border2_ + 1;
        total_ = ascii_ + 1;
    }
};
