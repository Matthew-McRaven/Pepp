#pragma once

#include "core/ds/opaque_handle.hpp"
#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"
namespace schematic {
using Coord = i16;
using Footprint = pepp::core::Rectangle<Coord>;
using Rectangle = pepp::core::Rectangle<Coord>;
using Point = pepp::core::Point<Coord>;
using Size = pepp::core::Size<Coord>;

template <typename T, typename V = u32> using Handle = pepp::OpaqueHandle<T, V>;
using BlueprintID = Handle<struct BlueprintIDTag>;
using BlueprintGroupID = Handle<struct BlueprintGroupIDTag>;
using LocalPinID = Handle<struct LocalPinIDTag>;
using ComponentID = Handle<struct ComponentIDTag>;
using ImageFileKey = Handle<struct ImageFileKeyTag>;
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

template <class Tag, class U> struct std::hash<schematic::Handle<Tag, U>> {
  size_t operator()(const schematic::Handle<Tag, U> &k) const noexcept { return std::hash<U>{}(k.value); }
};
