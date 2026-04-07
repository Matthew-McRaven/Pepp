#pragma once
#include "cnl/scaled_integer.h"
// cnl predate C++2020, so <=> is not natively supported.
namespace cnl {

template <typename T>
concept is_scaled_integer = requires(T t) { cnl::_impl::to_rep(t); } && !std::is_integral_v<T>;
template <typename Rep, typename Scale>
auto operator<=>(cnl::scaled_integer<Rep, Scale> const &lhs, cnl::scaled_integer<Rep, Scale> const &rhs) {
  return cnl::_impl::to_rep(lhs) <=> cnl::_impl::to_rep(rhs);
}

template <typename T> using rep_t = typename rep_of<T>::type;

// Helpers to extract the Rep and Scale from a scaled_integer, which are used in wider_type to recursively construct
// wider fixed point types.
template <typename T> struct scaled_integer_traits;
template <typename Rep, int Exponent, int Radix>
struct scaled_integer_traits<cnl::scaled_integer<Rep, cnl::power<Exponent, Radix>>> {
  using rep = Rep;
  using scale = cnl::power<Exponent, Radix>;
  static constexpr int exponent = Exponent;
  static constexpr int radix = Radix;
};
} // namespace cnl
