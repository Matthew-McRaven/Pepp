#pragma once

#include "core/integers.h"

namespace pepp {
// Tag is a phantom type that makes each instantiation a distinct type,
// so Handle<FooTag> and Handle<BarTag> cannot be mixed.
// You can make up any tag you want; the tag does not need an associated definition.
template <class Tag, class Underlying = u32> struct OpaqueHandle {
  using underlying_type = Underlying;
  Underlying value = 0;

  OpaqueHandle() = default;
  constexpr explicit OpaqueHandle(Underlying v) : value(v) {}

  auto operator<=>(const OpaqueHandle &) const = default;
  bool operator==(const OpaqueHandle &) const = default;

  constexpr bool operator!() const { return value == 0; }
  constexpr explicit operator bool() const { return value != 0; }
};
} // namespace pepp