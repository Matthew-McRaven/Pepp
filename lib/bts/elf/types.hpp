#pragma once
#include "bts/bitmanip/integers.h"
namespace pepp::bts {
// Data types which are conditionally endianness-reversed.
// E is an architecture, and it should have a member bool is_le;
template <typename E> using I16 = std::conditional_t<E::is_le, il16, ib16>;
template <typename E> using I32 = std::conditional_t<E::is_le, il32, ib32>;
template <typename E> using I64 = std::conditional_t<E::is_le, il64, ib64>;
template <typename E> using U16 = std::conditional_t<E::is_le, ul16, ub16>;
template <typename E> using U24 = std::conditional_t<E::is_le, ul24, ub24>;
template <typename E> using U32 = std::conditional_t<E::is_le, ul32, ub32>;
template <typename E> using U64 = std::conditional_t<E::is_le, ul64, ub64>;

template <typename E> using Word = std::conditional_t<E::is_64, U64<E>, U32<E>>;
template <typename E> using SWord = std::conditional_t<E::is_64, I64<E>, I32<E>>;

template <class Enum> constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(e);
}
} // namespace pepp::bts
