#pragma once
#include <QVariant>
#include "./expr_rtti.hpp"
#include "./expr_types.hpp"

namespace pepp::debug {

struct VNever : public types::Never {
  types::TypeInfo::DirectHandle type_handle = types::TypeInfo::DirectHandle();
  std::strong_ordering operator<=>(const VNever &other) const;
  bool operator==(const VNever &other) const;
};

struct VPrimitive : public types::Primitive {
  types::TypeInfo::DirectHandle type_handle;
  uint64_t bits;
  static VPrimitive with_bits(const VPrimitive &type, quint64);
  static VPrimitive promote(const VPrimitive &value, types::Primitives new_type);
  static VPrimitive from(types::Primitives new_type, quint64 bits);
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
    return VPrimitive{{type}, types::TypeInfo::DirectHandle(type), cast};
  }
  std::strong_ordering operator<=>(const VPrimitive &other) const;
  bool operator==(const VPrimitive &other) const;
};

struct VPointer {
  types::TypeInfo::DirectHandle type_handle;
  uint64_t bits;
  std::strong_ordering operator<=>(const VPointer &other) const;
  bool operator==(const VPointer &other) const;
};
struct VArray {
  types::TypeInfo::DirectHandle type_handle;
  uint64_t bits;
  std::strong_ordering operator<=>(const VArray &other) const;
  bool operator==(const VArray &other) const;
};
struct VStruct {
  types::TypeInfo::DirectHandle type_handle;
  uint64_t bits;
  std::strong_ordering operator<=>(const VStruct &other) const;
  bool operator==(const VStruct &other) const;
};
using Value = std::variant<VNever, VPrimitive, VPointer, VArray, VStruct>;
std::strong_ordering operator<=>(const Value &lhs, const Value &rhs);
bool operator==(const Value &lhs, const Value &rhs);

namespace details {
struct BitVisitor {
  uint64_t operator()(const VNever &v) const { return 0; }
  uint64_t operator()(const VPrimitive &v) const { return v.bits; }
  uint64_t operator()(const VPointer &v) const { return v.bits; }
  uint64_t operator()(const VArray &v) const { return v.bits; }
  uint64_t operator()(const VStruct &v) const { return v.bits; }
};
} // namespace details
template <std::integral T = uint64_t> T value_bits(const Value &v) {
  auto ret = std::visit(details::BitVisitor{}, v);
  return static_cast<T>(ret);
}
QVariant from_bits(const Value &v);
Value from_bits(const types::Type &, quint64 bits);

// Overloading not possible since they need an extra argument
namespace operators {
// Type ops
types::Type op1_typeof(const types::TypeInfo &info, const Value &v);
types::BoxedType op1_dereference_typeof(const types::TypeInfo &info, const Value &v);
Value op2_typecast(const types::TypeInfo &info, const Value &from, const types::BoxedType &to);
// Unary arithmetic ops
Value op1_plus(const types::TypeInfo &info, const Value &v);
Value op1_minus(const types::TypeInfo &info, const Value &v);
// Unary bitwise ops
Value op1_not(const types::TypeInfo &info, const Value &v);
Value op1_negate(const types::TypeInfo &info, const Value &v);
// Unary memory ops
Value op1_dereference(const types::TypeInfo &info, const Value &v);
Value op1_addressof(const types::TypeInfo &info, const Value &v);
// Binary arithmetic ops
Value op2_add(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_sub(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_mul(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_div(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_mod(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
// Binary Bitwise ops
Value op2_bsl(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_bsr(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_bitand(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_bitor(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_bitxor(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
// Binary comparison ops, all implemented in terms of <=>
Value op2_spaceship(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_lt(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_le(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_eq(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_ne(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_ge(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
Value op2_gt(const types::TypeInfo &info, const Value &lhs, const Value &rhs);
} // namespace operators
} // namespace pepp::debug
