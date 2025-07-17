#include <stdexcept>
#include "./expr_value.hpp"

namespace detail {
using namespace pepp::debug;
// Try to swap order of arguments. If you do not want the arguments to be swapped, you will need to poison those
// overloads.
template <typename T> struct SwapDispatch {
  const types::TypeInfo &info;
  Value operator()(const auto &lhs, const auto &rhs) const {
    if constexpr (requires { T{info}(lhs, rhs); }) {
      return T{info}(lhs, rhs);
    } else if constexpr (requires { T{info}(rhs, lhs); }) return T{}(rhs, lhs);
    else throw std::runtime_error("unsupported operand types for operation");
  }
};

struct BinaryUnimplmenetedVisitor {
  Value operator()(const auto &, const auto &) const { return VNever{}; }
};

struct BinaryTypecastVisitor {
  const types::TypeInfo &info;
  Value operator()(const VNever &from, const auto &to) const { return VNever{}; }
  Value operator()(const VPrimitive &from, const types::Primitive &to) const {
    if (from.primitive == to.primitive) return from;
    return VPrimitive::from(to.primitive, from.bits);
  }
  Value operator()(const VPrimitive &from, const types::Pointer &to) const {
    types::Type to_copy = to;
    auto hnd = info.get_direct(to_copy);
    if (!hnd) return VNever{};
    auto bits = pepp::debug::types::mask_pointer_bits(to.pointer_size, from.bits);
    return VPointer{*hnd, bits};
  }
  Value operator()(const VPrimitive &from, const types::Array &to) const {
    types::Type to_copy = to;
    auto hnd = info.get_direct(to_copy);
    if (!hnd) return VNever{};
    auto bits = pepp::debug::types::mask_pointer_bits(to.pointer_size, from.bits);
    return VArray{*hnd, bits};
  }
  Value operator()(const VPrimitive &from, const types::Struct &to) const {
    types::Type to_copy = to;
    auto hnd = info.get_direct(to_copy);
    if (!hnd) return VNever{};
    auto bits = pepp::debug::types::mask_pointer_bits(to.pointer_size, from.bits);
    return VStruct{*hnd, bits};
  }
  Value operator()(const auto &, const auto &) const { return VNever{}; }
};

template <typename Op> struct BinaryArithVisitor {
  const types::TypeInfo &info;
  Value operator()(const VNever &lhs, const auto &rhs) const { return VNever{}; }
  Value operator()(const VPrimitive &lhs, const VPrimitive &rhs) const {
    if (lhs.primitive == rhs.primitive) return VPrimitive::with_bits(lhs, Op{}(lhs.bits, rhs.bits));
    auto common = types::common_type(lhs.primitive, rhs.primitive);
    return (*this)(VPrimitive::promote(lhs, common), VPrimitive::promote(rhs, common));
  }
  Value operator()(const auto &, const auto &) const { return VNever{}; }
};

struct BinaryPlusVisitor : public BinaryArithVisitor<std::plus<uint64_t>> {
  using BinaryArithVisitor<std::plus<uint64_t>>::operator();
  Value operator()(const VPointer &lhs, const VPrimitive &rhs) const { return VNever{}; }
  Value operator()(const VArray &lhs, const VPrimitive &rhs) const { return VNever{}; }
};

struct BinaryMinusVisitor : public BinaryArithVisitor<std::minus<uint64_t>> {
  using BinaryArithVisitor<std::minus<uint64_t>>::operator();
  Value operator()(const VPointer &lhs, const VPrimitive &rhs) const { return VNever{}; }
  Value operator()(const VArray &lhs, const VPrimitive &rhs) const { return VNever{}; }
};

struct BinaryTimesVisitor : public BinaryArithVisitor<std::multiplies<uint64_t>> {
  using BinaryArithVisitor<std::multiplies<uint64_t>>::operator();
};

struct BinaryDivideVisitor : public BinaryArithVisitor<std::divides<uint64_t>> {
  using BinaryArithVisitor<std::divides<uint64_t>>::operator();
};

struct BinaryModuloVisitor : public BinaryArithVisitor<std::modulus<uint64_t>> {
  using BinaryArithVisitor<std::modulus<uint64_t>>::operator();
};

struct bit_shift_left {
  uint64_t operator()(uint64_t x, uint64_t y) const { return x << y; }
};

struct bit_shift_right {
  uint64_t operator()(uint64_t x, uint64_t y) const { return x >> y; }
};

struct BinaryShiftLeftVisitor : public BinaryArithVisitor<bit_shift_left> {
  using BinaryArithVisitor<bit_shift_left>::operator();
};

struct BinaryShiftRightVisitor : public BinaryArithVisitor<bit_shift_right> {
  using BinaryArithVisitor<bit_shift_right>::operator();
};

struct BinaryANDVisitor : public BinaryArithVisitor<std::bit_and<uint64_t>> {
  using BinaryArithVisitor<std::bit_and<uint64_t>>::operator();
};

struct BinaryORVisitor : public BinaryArithVisitor<std::bit_or<uint64_t>> {
  using BinaryArithVisitor<std::bit_or<uint64_t>>::operator();
};

struct BinaryXORVisitor : public BinaryArithVisitor<std::bit_xor<uint64_t>> {
  using BinaryArithVisitor<std::bit_xor<uint64_t>>::operator();
};
} // namespace detail

pepp::debug::Value pepp::debug::operators::op2_typecast(const types::TypeInfo &info, const Value &from,
                                                        const types::BoxedType &to) {
  auto type = types::unbox(to);
  return std::visit(::detail::BinaryTypecastVisitor{info}, from, type);
}

pepp::debug::Value pepp::debug::operators::op2_add(const types::TypeInfo &info, const Value &lhs,
                                                   const Value &rhs) {
  return std::visit(::detail::SwapDispatch<::detail::BinaryPlusVisitor>{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_sub(const types::TypeInfo &info, const Value &lhs,
                                                   const Value &rhs) {
  return std::visit(::detail::BinaryMinusVisitor{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_mul(const types::TypeInfo &info, const Value &lhs,
                                                   const Value &rhs) {
  return std::visit(::detail::SwapDispatch<::detail::BinaryTimesVisitor>{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_div(const types::TypeInfo &info, const Value &lhs,
                                                   const Value &rhs) {
  return std::visit(::detail::BinaryDivideVisitor{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_mod(const types::TypeInfo &info, const Value &lhs,
                                                   const Value &rhs) {
  return std::visit(::detail::BinaryModuloVisitor{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_bsl(const types::TypeInfo &info, const Value &lhs,
                                                   const Value &rhs) {
  return std::visit(::detail::BinaryShiftLeftVisitor{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_bsr(const types::TypeInfo &info, const Value &lhs,
                                                   const Value &rhs) {
  return std::visit(::detail::BinaryShiftRightVisitor{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_bitand(const types::TypeInfo &info, const Value &lhs,
                                                      const Value &rhs) {
  return std::visit(::detail::SwapDispatch<::detail::BinaryANDVisitor>{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_bitor(const types::TypeInfo &info, const Value &lhs,
                                                     const Value &rhs) {
  return std::visit(::detail::SwapDispatch<::detail::BinaryORVisitor>{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_bitxor(const types::TypeInfo &info, const Value &lhs,
                                                      const Value &rhs) {
  return std::visit(::detail::SwapDispatch<::detail::BinaryXORVisitor>{info}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::op2_spaceship(const types::TypeInfo &info, const Value &lhs,
                                                         const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  int8_t result = ret == std::strong_ordering::less ? -1 : (ret == std::strong_ordering::equal ? 0 : 1);
  return VPrimitive::i8(result);
}

pepp::debug::Value pepp::debug::operators::op2_lt(const types::TypeInfo &info, const Value &lhs,
                                                  const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret == std::strong_ordering::less) return VPrimitive::True();
  else return VPrimitive::False();
}

pepp::debug::Value pepp::debug::operators::op2_le(const types::TypeInfo &info, const Value &lhs,
                                                  const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret != std::strong_ordering::greater) return VPrimitive::True();
  else return VPrimitive::False();
}

pepp::debug::Value pepp::debug::operators::op2_eq(const types::TypeInfo &info, const Value &lhs,
                                                  const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret == std::strong_ordering::equal) return VPrimitive::True();
  else return VPrimitive::False();
}

pepp::debug::Value pepp::debug::operators::op2_ne(const types::TypeInfo &info, const Value &lhs,
                                                  const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret != std::strong_ordering::equal) return VPrimitive::True();
  else return VPrimitive::False();
}

pepp::debug::Value pepp::debug::operators::op2_ge(const types::TypeInfo &info, const Value &lhs,
                                                  const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret != std::strong_ordering::less) return VPrimitive::True();
  else return VPrimitive::False();
}

pepp::debug::Value pepp::debug::operators::op2_gt(const types::TypeInfo &info, const Value &lhs,
                                                  const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret == std::strong_ordering::greater) return VPrimitive::True();
  else return VPrimitive::False();
}
