#pragma once
#include "bts/bitmanip/integers.h"
namespace pepp::bts {

enum class ElfBits : u8 { b32, b64 };
enum class ElfEndian : u8 { le, be };

// Data types which are conditionally endianness-reversed.
// E is an architecture, and it should have a member bool is_le;
template <ElfEndian E> using I16 = std::conditional_t<E == ElfEndian::le, il16, ib16>;
template <ElfEndian E> using I32 = std::conditional_t<E == ElfEndian::le, il32, ib32>;
template <ElfEndian E> using I64 = std::conditional_t<E == ElfEndian::le, il64, ib64>;
template <ElfEndian E> using U16 = std::conditional_t<E == ElfEndian::le, ul16, ub16>;
template <ElfEndian E> using U24 = std::conditional_t<E == ElfEndian::le, ul24, ub24>;
template <ElfEndian E> using U32 = std::conditional_t<E == ElfEndian::le, ul32, ub32>;
template <ElfEndian E> using U64 = std::conditional_t<E == ElfEndian::le, ul64, ub64>;

template <ElfBits B, ElfEndian E> using Word = std::conditional_t<B == ElfBits::b64, U64<E>, U32<E>>;
template <ElfBits B, ElfEndian E> using SWord = std::conditional_t<B == ElfBits::b64, I64<E>, I32<E>>;

template <class Enum> constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(e);
}
} // namespace pepp::bts
