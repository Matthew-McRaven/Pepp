#include "./expr_value2.hpp"

// Try to swap order of arguments. If you do not want the arguments to be swapped, you will need to poison those
// overloads.
template <typename T> struct SwapDispatch {
  pepp::debug::Value operator()(const auto &lhs, const auto &rhs) const {
    if constexpr (requires { T{}(lhs, rhs); }) {
      return T{}(lhs, rhs);
    } else if constexpr (requires { T{}(rhs, lhs); }) return T{}(rhs, lhs);
    else throw std::runtime_error("unsupported operand types for operation");
  }
};

struct BinaryUnimplmenetedVisitor {
  pepp::debug::Value operator()(const auto &, const auto &) const { return pepp::debug::VNever{}; }
};
template <typename Op> struct BinaryArithVisitor {
  pepp::debug::Value operator()(const pepp::debug::VNever &lhs, const auto &rhs) const { return pepp::debug::VNever{}; }
  pepp::debug::Value operator()(const pepp::debug::VPrimitive &lhs, const pepp::debug::VPrimitive &rhs) const {
    if (lhs.primitive == rhs.primitive) return pepp::debug::VPrimitive::with_bits(lhs, Op{}(lhs.bits, rhs.bits));
    auto common = pepp::debug::types::common_type(lhs.primitive, rhs.primitive);
    return (*this)(pepp::debug::VPrimitive::promote(lhs, common), pepp::debug::VPrimitive::promote(rhs, common));
  }
  pepp::debug::Value operator()(const auto &, const auto &) const { return pepp::debug::VNever{}; }
};

struct BinaryPlusVisitor : public BinaryArithVisitor<std::plus<uint64_t>> {
  using BinaryArithVisitor<std::plus<uint64_t>>::operator();
  pepp::debug::Value operator()(const pepp::debug::VPointer &lhs, const pepp::debug::VPrimitive &rhs) const {
    return pepp::debug::VNever{};
  }
  pepp::debug::Value operator()(const pepp::debug::VArray &lhs, const pepp::debug::VPrimitive &rhs) const {
    return pepp::debug::VNever{};
  }
};

struct BinaryMinusVisitor : public BinaryArithVisitor<std::minus<uint64_t>> {
  using BinaryArithVisitor<std::minus<uint64_t>>::operator();
  pepp::debug::Value operator()(const pepp::debug::VPointer &lhs, const pepp::debug::VPrimitive &rhs) const {
    return pepp::debug::VNever{};
  }
  pepp::debug::Value operator()(const pepp::debug::VArray &lhs, const pepp::debug::VPrimitive &rhs) const {
    return pepp::debug::VNever{};
  }
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

pepp::debug::Value pepp::debug::operators::operator+(const Value &lhs, const Value &rhs) {
  return std::visit(SwapDispatch<BinaryPlusVisitor>{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator-(const Value &lhs, const Value &rhs) {
  return std::visit(BinaryMinusVisitor{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator*(const Value &lhs, const Value &rhs) {
  return std::visit(SwapDispatch<BinaryTimesVisitor>{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator/(const Value &lhs, const Value &rhs) {
  return std::visit(BinaryDivideVisitor{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator%(const Value &lhs, const Value &rhs) {
  return std::visit(BinaryModuloVisitor{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator<<(const Value &lhs, const Value &rhs) {
  return std::visit(BinaryShiftLeftVisitor{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator>>(const Value &lhs, const Value &rhs) {
  return std::visit(BinaryShiftRightVisitor{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator&(const Value &lhs, const Value &rhs) {
  return std::visit(SwapDispatch<BinaryANDVisitor>{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator|(const Value &lhs, const Value &rhs) {
  return std::visit(SwapDispatch<BinaryORVisitor>{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::operators::operator^(const Value &lhs, const Value &rhs) {
  return std::visit(SwapDispatch<BinaryXORVisitor>{}, lhs, rhs);
}

pepp::debug::Value pepp::debug::compare::operator<=>(const Value &lhs, const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  int8_t result = ret == std::strong_ordering::less ? -1 : (ret == std::strong_ordering::equal ? 0 : 1);
  return pepp::debug::VPrimitive::i8(result);
}

pepp::debug::Value pepp::debug::compare::operator<(const Value &lhs, const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret == std::strong_ordering::less) return pepp::debug::VPrimitive::True();
  else return pepp::debug::VPrimitive::False();
}

pepp::debug::Value pepp::debug::compare::operator<=(const Value &lhs, const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret != std::strong_ordering::greater) return pepp::debug::VPrimitive::True();
  else return pepp::debug::VPrimitive::False();
}

pepp::debug::Value pepp::debug::compare::operator==(const Value &lhs, const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret == std::strong_ordering::equal) return pepp::debug::VPrimitive::True();
  else return pepp::debug::VPrimitive::False();
}

pepp::debug::Value pepp::debug::compare::operator!=(const Value &lhs, const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret != std::strong_ordering::equal) return pepp::debug::VPrimitive::True();
  else return pepp::debug::VPrimitive::False();
}

pepp::debug::Value pepp::debug::compare::operator>=(const Value &lhs, const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret != std::strong_ordering::less) return pepp::debug::VPrimitive::True();
  else return pepp::debug::VPrimitive::False();
}

pepp::debug::Value pepp::debug::compare::operator>(const Value &lhs, const Value &rhs) {
  auto ret = ::pepp::debug::operator<=>(lhs, rhs);
  if (ret == std::strong_ordering::greater) return pepp::debug::VPrimitive::True();
  else return pepp::debug::VPrimitive::False();
}
