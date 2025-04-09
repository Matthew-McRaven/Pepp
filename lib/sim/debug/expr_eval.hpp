#pragma once
#include <QtCore>
#include <memory>
#include <set>
namespace pepp::debug {

enum class ExpressionType : uint8_t { i8, u8, i16, u16, i32, u32 };
bool is_unsigned(ExpressionType t);
// Returns the number of bits in t.
uint32_t bitness(ExpressionType t);
ExpressionType common_type(ExpressionType rhs, ExpressionType lhs);

struct TypedBits {
  bool allows_address_of = false;
  ExpressionType type;
  uint64_t bits;

  std::strong_ordering operator<=>(const TypedBits &rhs) const;

  friend pepp::debug::TypedBits operator+(const pepp::debug::TypedBits &arg);
  friend pepp::debug::TypedBits operator-(const pepp::debug::TypedBits &arg);
  // Operators I do not want to overload because I'm scared I'll break a C++ container
  friend pepp::debug::TypedBits _deref(const pepp::debug::TypedBits &arg);
  friend pepp::debug::TypedBits _addressof(const pepp::debug::TypedBits &arg);
  friend pepp::debug::TypedBits operator!(const pepp::debug::TypedBits &arg);
  friend pepp::debug::TypedBits operator~(const pepp::debug::TypedBits &arg);
  friend pepp::debug::TypedBits operator*(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator/(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator%(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator+(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator-(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator<<(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator>>(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  // Operators I do not want to overload because I'm afraid of interactions with <=> and not being a bool.
  friend pepp::debug::TypedBits _lt(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits _le(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits _eq(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits _ne(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits _gt(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits _ge(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator&(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator|(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
  friend pepp::debug::TypedBits operator^(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs);
};

template <std::integral I> TypedBits from_int(I bits) {
  using T = pepp::debug::ExpressionType;
  static_assert(sizeof(I) <= 4);
  T type;
  auto cast = static_cast<uint64_t>(bits);
  if constexpr (std::is_same_v<I, int8_t>) type = T::i8;
  else if constexpr (std::is_same_v<I, uint8_t>) type = T::u8;
  else if constexpr (std::is_same_v<I, int16_t>) type = T::i16;
  else if constexpr (std::is_same_v<I, uint16_t>) type = T::u16;
  else if constexpr (std::is_same_v<I, int32_t>) type = T::i32;
  else if constexpr (std::is_same_v<I, uint32_t>) type = T::u32;
  return TypedBits{.allows_address_of = false, .type = type, .bits = cast};
}
TypedBits with_bits(const TypedBits &type, uint64_t new_value);
TypedBits promote(const TypedBits &value, ExpressionType as_type);

enum class EvaluationMode { UseCache, RecomputeSelf, RecomputeTree };
EvaluationMode mode_for_child(EvaluationMode current);

// Variables, registers are volatile.
// All volatile "things" must be updated manually at the end of each simulator step
// Then you can evaluate your top level expressions and re-generate caches if values changed.
// This prevents having to recursively evaluate the entire tree to check for a volatile change.
struct CVQualifiers {
  enum { Constant = 0x1, Volatile = 0x2 };
};

struct EvaluationCache {
  bool dirty = false;
  std::optional<TypedBits> value = std::nullopt;
};
} // namespace pepp::debug

