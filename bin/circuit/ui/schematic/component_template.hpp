#pragma once
#include <vector>
#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"

enum class PinType : u8 {
  HighZ = 0,
  Input,
  Output,
  Clock,
};

struct ComponentTemplate {
  struct Pin {
    // Position is rleative to the geometry of the containing component.
    pepp::core::Rectangle<i16> geometry;
    PinType type = PinType::HighZ;
  };

  pepp::core::Rectangle<i16> geometry;
  std::vector<Pin> pins;

  u16 input_pins() const;
  u16 output_pins() const;
  u16 clock_pins() const;
};

// A an atomic component which cannot be broken down any futher
struct BuiltinComponentTemplate : public ComponentTemplate {
  enum class Type {
    Buffer,
    AND,
    OR,
    NOT,
    NAND,
    NOR,
    XOR,
    XNOR,
    CrossCoupledNAND,
    CrossCoupledNOR,
  } type = Type::Buffer;
};

// A component which is itself composed of other components. For example, a 2-input multiplexer.
struct BlockComponentTemplate : public ComponentTemplate {};

// A group of related component templates.
// For example, each AND gate with a different number of inputs counts as a separate template.
// However, the property editor should allow you to pick between the various allowed templates within the family.
struct ComponentFamily {
  std::vector<ComponentTemplate> templates;
};
