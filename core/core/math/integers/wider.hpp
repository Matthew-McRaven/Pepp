#pragma once

#include <concepts>
#include "core/integers.h"
#include "core/math/integers/fixed_point_utils.hpp"

namespace pepp::core {

// Recursive template to find the next wider integer type.
template <bool Signed, std::size_t Size> struct wider_int;
template <> struct wider_int<false, 1> {
  using type = u16;
};
template <> struct wider_int<false, 2> {
  using type = u32;
};
template <> struct wider_int<false, 4> {
  using type = u64;
};
template <> struct wider_int<true, 1> {
  using type = u16;
};
template <> struct wider_int<true, 2> {
  using type = u32;
};
template <> struct wider_int<true, 4> {
  using type = u64;
};

template <typename T> struct wider_type;
// Specialization for integrals
template <std::integral T> struct wider_type<T> {
  static_assert(sizeof(T) <= 4, "no wider integer available");
  using type = typename wider_int<std::is_signed_v<T>, sizeof(T)>::type;
};

// Specialization for scale_integers, which will call wider_type recursively for integral.
template <typename T>
  requires requires { typename cnl::scaled_integer_traits<T>::rep; }
struct wider_type<T> {
  using traits = cnl::scaled_integer_traits<T>;
  using type = cnl::scaled_integer<typename wider_type<typename traits::rep>::type, typename traits::scale>;
};
template <typename Rep, typename Scale> struct wider_type<cnl::_impl::wrapper<Rep, Scale>> {
  using type = cnl::_impl::wrapper<typename wider_type<Rep>::type, Scale>;
};
// Convenience to extract the type via wider_type::type
template <typename T> using wider_type_t = typename wider_type<T>::type;

} // namespace pepp::core
