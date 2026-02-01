#pragma once

#include <QMetaType> // Required for Q_DECLARE_METATYPE

class DiagramType
{
    Q_GADGET

public:
    enum Type : quint32 {
        ANDGate = 0,
        ORGate,
        Inverter,
        NANDGate,
        NORGate,
        XORGate,
        //XNORGate,

        //  Line enums
        Line,
        MultiLine,
        Bus,

        //  Indicates invalid state from parsing input files
        Invalid = 0xffffffff,
    };
    Q_ENUM(Type)
};
