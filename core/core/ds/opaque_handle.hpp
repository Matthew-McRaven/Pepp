#pragma once

#include "core/integers.h"

namespace pepp {

// For each opaque handle you want to allow pre/post increment on, define:
// consteval void allow_opaque_handle_increment(MYTYPE);
template <typename Handle>
concept HandleCanIncrement = requires(Handle h) { allow_opaque_handle_increment(h); };
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

  // Pre-increment and post-increment only enabled via ADL
  friend constexpr OpaqueHandle &operator++(OpaqueHandle &h)
    requires HandleCanIncrement<OpaqueHandle>
  {
    ++h.value;
    return h;
  }
  friend constexpr OpaqueHandle operator++(OpaqueHandle &&h)
    requires HandleCanIncrement<OpaqueHandle>
  {
    ++h.value;
    return h;
  }

  friend constexpr OpaqueHandle operator++(OpaqueHandle &h, int)
    requires HandleCanIncrement<OpaqueHandle>
  {
    OpaqueHandle old = h;
    ++h.value;
    return old;
  }
};
} // namespace pepp