#pragma once

#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"
namespace schematic {
using Coord = i16;
using Footprint = pepp::core::Rectangle<Coord>;
using Rectangle = pepp::core::Rectangle<Coord>;
using Point = pepp::core::Point<Coord>;
using Size = pepp::core::Size<Coord>;

// Tag is a phantom type that makes each instantiation a distinct type,
// so Handle<FooTag> and Handle<BarTag> cannot be mixed.
// You can make up any tag you want; the tag does not need an associated definition.
template <class Tag, class Underlying = u32> struct Handle {
  using underlying_type = Underlying;
  Underlying value = 0;

  Handle() = default;
  constexpr explicit Handle(Underlying v) : value(v) {}

  auto operator<=>(const Handle &) const = default;
  bool operator==(const Handle &) const = default;

  constexpr bool operator!() const { return value == 0; }
  constexpr explicit operator bool() const { return value != 0; }
};

using BlueprintID = Handle<struct BlueprintIDTag>;
using LocalPinID = Handle<struct LocalPinIDTag>;
using ComponentID = Handle<struct ComponentIDTag>;
using MipmapStoreKey = Handle<struct MipmapStoreKeyTag>;

struct GlobalPinID {
  GlobalPinID() = default;
  inline explicit GlobalPinID(u64 id) : component_id(id >> 32), local_pin_id(id & 0xFFFFFFFF) {}
  GlobalPinID(ComponentID component_id, LocalPinID local_pin_id)
      : component_id(component_id), local_pin_id(local_pin_id) {}
  ComponentID component_id;
  LocalPinID local_pin_id;
  auto operator<=>(const GlobalPinID &) const = default;
  bool operator==(const GlobalPinID &) const = default;
};
} // namespace schematic

namespace std {
template <class Tag, class U> struct std::hash<schematic::Handle<Tag, U>> {
  size_t operator()(const schematic::Handle<Tag, U> &k) const noexcept { return std::hash<U>{}(k.value); }
};
} // namespace std
