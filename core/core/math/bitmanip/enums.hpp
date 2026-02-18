#pragma once

#include <type_traits>
namespace bits {
// Will eventually exist in C++23, but it can be implemented easily enough by hand until it is widely supported.
template <class Enum> constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(e);
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator|(const T lhs, const T rhs) {
  return static_cast<T>(to_underlying(lhs) | to_underlying(rhs));
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator|=(T &lhs, const T &rhs) {
  lhs = lhs | rhs;
  return lhs;
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator&(const T lhs, const T rhs) {
  return static_cast<T>(to_underlying(lhs) & to_underlying(rhs));
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator&=(T &lhs, const T &rhs) {
  lhs = lhs & rhs;
  return lhs;
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator^(const T lhs, const T rhs) {
  return static_cast<T>(to_underlying(lhs) ^ to_underlying(rhs));
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator^=(T &lhs, const T &rhs) {
  lhs = lhs ^ rhs;
  return lhs;
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator~(const T lhs) {
  return static_cast<T>(~to_underlying(lhs));
}

// Can be hijacked to provide a bool conversion.
// !!e will yield a bool; we can't have a free operator bool() on enums
template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator!(const T lhs) {
  return static_cast<T>(!to_underlying(lhs));
}

// Return true if any bits are set.
// We can't have a free operator bool() on enums, so this is the next closest thing
template <class T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr bool any(T lhs) noexcept {
  return to_underlying(lhs) != 0;
}

template <class T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr bool none(T lhs) noexcept {
  return to_underlying(lhs) == 0;
}

} // namespace bits
