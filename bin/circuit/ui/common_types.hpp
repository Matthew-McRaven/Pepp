#pragma once

#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"
namespace schematic {
using Coord = i16;
using Footprint = pepp::core::Rectangle<Coord>;
using Rectangle = pepp::core::Rectangle<Coord>;
using Point = pepp::core::Point<Coord>;
using Size = pepp::core::Size<Coord>;
struct BlueprintID {
  BlueprintID() = default;
  inline explicit BlueprintID(u32 id) : value(id) {}
  u32 value = 0;
  auto operator<=>(const BlueprintID &) const = default;
  bool operator==(const BlueprintID &) const = default;
};
struct LocalPinID {
  LocalPinID() = default;
  inline explicit LocalPinID(u32 id) : value(id) {}
  u32 value = 0;
  auto operator<=>(const LocalPinID &) const = default;
  bool operator==(const LocalPinID &) const = default;
};
struct ComponentID {
  ComponentID() = default;
  inline explicit ComponentID(u32 id) : value(id) {}
  u32 value = 0;
  auto operator<=>(const ComponentID &) const = default;
  bool operator==(const ComponentID &) const = default;
  inline bool operator!() const { return value == 0; }
};
struct MipmapStoreKey {
  MipmapStoreKey() = default;
  inline explicit MipmapStoreKey(u32 id) : value(id) {}
  u32 value = 0;
  auto operator<=>(const MipmapStoreKey &) const = default;
  bool operator==(const MipmapStoreKey &) const = default;
  inline bool operator!() const { return value == 0; }
};
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
template <> struct std::hash<schematic::MipmapStoreKey> {
  size_t operator()(const schematic::MipmapStoreKey &k) const noexcept { return std::hash<u32>{}(k.value); }
};
} // namespace std
