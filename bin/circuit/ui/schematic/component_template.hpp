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

struct AlignmentConstraint {
  // Only allow the component to be placed at pos.x%x_modulus == x_offset, and similarly for y.
  // While a component can also be aligned to any (1,1) in instructor mode, this is used to make snap-to alignment.
  // easier in practice.
  // Modulus must always be >0 and offset+1 <modules.
  // A modulus of 1 effectively disables alignment constraints.
  i16 x_modulus = 1, x_offset = 0;
  i16 y_modulus = 1, y_offset = 0;

  pepp::core::Point<i16> nearest_aligned_point(const pepp::core::Point<i16> &pt) const noexcept;
  bool is_aligned(const pepp::core::Point<i16> &pt) const noexcept;
};

struct ComponentTemplate {
  struct Pin {
    // Position is rleative to the geometry of the containing component.
    pepp::core::Rectangle<i16> geometry;
    PinType type = PinType::HighZ;
  };

  pepp::core::Rectangle<i16> geometry;
  std::vector<Pin> pins;
  AlignmentConstraint alignmentConstraint;

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
