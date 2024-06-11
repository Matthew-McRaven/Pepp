#pragma once

#include <QObject>

class Themes {
  Q_GADGET
public:
  enum Roles {
    BaseRole = 0,
    WindowRole,
    ButtonRole,
    HighlightRole,
    TooltipRole,
    AlternateBaseRole,
    AccentRole,
    LightRole,
    MidLightRole,
    MidRole,
    DarkRole,
    ShadowRole,
    LinkRole,
    LinkVisitedRole,
    BrightTextRole,
    PlaceHolderTextRole,

    //  Editor enums
    RowNumberRole,
    BreakpointRole,

    //  Circuit enums
    SeqCircuitRole,
    CircuitGreenRole,

    Total, // Must be last
    //  Indicates invalid state from parsing input files
    Invalid = 0xffffffff,
  };
  Q_ENUM(Roles)

};
