#pragma once
#include "./expr_types.hpp"

namespace pepp::debug {

struct VNever : public types::TNever {};
struct VPrimitive : public types::TPrimitive {
  uint64_t bits;
  static VPrimitive with_bits(const VPrimitive &type, quint64);
  static VPrimitive promote(const VPrimitive &value, types::Primitives new_type);
  static VPrimitive True();
  static VPrimitive False();
  static VPrimitive i8(int8_t v);
  template <std::integral I> static VPrimitive from_int(I bits) {
    using T = types::Primitives;
    static_assert(sizeof(I) <= 4);
    T type;
    auto cast = static_cast<uint64_t>(bits);
    if constexpr (std::is_same_v<I, int8_t>) type = T::i8;
    else if constexpr (std::is_same_v<I, uint8_t>) type = T::u8;
    else if constexpr (std::is_same_v<I, int16_t>) type = T::i16;
    else if constexpr (std::is_same_v<I, uint16_t>) type = T::u16;
    else if constexpr (std::is_same_v<I, int32_t>) type = T::i32;
    else if constexpr (std::is_same_v<I, uint32_t>) type = T::u32;
    return VPrimitive{{type}, cast};
  }
};

struct VPointer : public types::TPointer {
  uint64_t bits;
  uint8_t width;
};
struct VArray : public types::TArray {
  uint64_t bits;
};
struct VStruct : public types::TStruct {
  uint64_t bits;
};
using Value = std::variant<VNever, VPrimitive, VPointer, VArray, VStruct>;
std::strong_ordering operator<=>(const Value &lhs, const Value &rhs);
namespace details {
struct Bitvisitor {
  uint64_t operator()(const VNever &v) const { return 0; }
  uint64_t operator()(const VPrimitive &v) const { return v.bits; }
  uint64_t operator()(const VPointer &v) const { return v.bits; }
  uint64_t operator()(const VArray &v) const { return v.bits; }
  uint64_t operator()(const VStruct &v) const { return v.bits; }
};
} // namespace details
template <std::integral T> T value_bits(const Value &v) {
  auto ret = std::visit(details::Bitvisitor{}, v);
  return static_cast<T>(ret);
}
types::Type _typeof(const Value &v);
// Since these operators are in their own namespace, I'm less terrified about operator overloading
namespace operators {
// Unary arithmetic ops
Value operator+(const Value &v);
Value operator-(const Value &v);
// Unary bitwise ops
Value operator!(const Value &v);
Value operator~(const Value &v);
// Unary memory ops
Value operator*(const Value &v);
Value operator&(const Value &v);
// Binary arithmetic ops
Value operator+(const Value &lhs, const Value &rhs);
Value operator-(const Value &lhs, const Value &rhs);
Value operator*(const Value &lhs, const Value &rhs);
Value operator/(const Value &lhs, const Value &rhs);
Value operator%(const Value &lhs, const Value &rhs);
// Binary Bitwise ops
Value operator<<(const Value &lhs, const Value &rhs);
Value operator>>(const Value &lhs, const Value &rhs);
Value operator&(const Value &lhs, const Value &rhs);
Value operator|(const Value &lhs, const Value &rhs);
Value operator^(const Value &lhs, const Value &rhs);
} // namespace operators
// Seperate namespace so you can use operators without all bin ops catching on fire.
namespace compare {
// Binary comparison ops, all implemented in terms of <=>
Value operator<=>(const Value &lhs, const Value &rhs);
Value operator<(const Value &lhs, const Value &rhs);
Value operator<=(const Value &lhs, const Value &rhs);
Value operator==(const Value &lhs, const Value &rhs);
Value operator!=(const Value &lhs, const Value &rhs);
Value operator>=(const Value &lhs, const Value &rhs);
Value operator>(const Value &lhs, const Value &rhs);
} // namespace compare
} // namespace pepp::debug
