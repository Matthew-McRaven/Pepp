#include "expr_eval.hpp"

pepp::debug::EvaluationMode pepp::debug::mode_for_child(EvaluationMode current) {
  switch (current) {
  case EvaluationMode::UseCache: [[fallthrough]];
  case EvaluationMode::RecomputeSelf: return EvaluationMode::UseCache;
  case EvaluationMode::RecomputeTree: return EvaluationMode::RecomputeTree;
  }
}
bool pepp::debug::is_unsigned(ExpressionType t) {
  switch (t) {
  case pepp::debug::ExpressionType::i8: [[fallthrough]];
  case pepp::debug::ExpressionType::i16: [[fallthrough]];
  case pepp::debug::ExpressionType::i32: return false;
  case pepp::debug::ExpressionType::u8: [[fallthrough]];
  case pepp::debug::ExpressionType::u16: [[fallthrough]];
  case pepp::debug::ExpressionType::u32: return true;
  }
}

uint32_t pepp::debug::bitness(ExpressionType t) {
  switch (t) {
  case pepp::debug::ExpressionType::i8: [[fallthrough]];
  case pepp::debug::ExpressionType::u8: return 8;
  case pepp::debug::ExpressionType::i16: [[fallthrough]];
  case pepp::debug::ExpressionType::u16: return 16;
  case pepp::debug::ExpressionType::i32: [[fallthrough]];
  case pepp::debug::ExpressionType::u32: return 32;
  }
}
pepp::debug::ExpressionType pepp::debug::common_type(ExpressionType rhs, ExpressionType lhs) {
  if (lhs == rhs) return lhs;

  auto lhs_bitness = bitness(lhs), rhs_bitness = bitness(rhs);
  auto lhs_unsigned = is_unsigned(lhs), rhs_unsigned = is_unsigned(rhs);
  // TODO: I think these rules can be simplified to reduce the amount of branching.
  // If both share a sign, pick the larger of the two types.
  if (lhs_unsigned == rhs_unsigned) {
    if (lhs_bitness > rhs_bitness) return lhs;
    return rhs;
  }
  // If only one is signed, prefer the unsigned type unless the signed type is bigger.
  else if (lhs_unsigned) {
    if (lhs_bitness >= rhs_bitness) return lhs;
    return rhs;
  } else {
    if (rhs_bitness >= lhs_bitness) return rhs;
    return lhs;
  }
}
std::strong_ordering pepp::debug::TypedBits::operator<=>(const TypedBits &rhs) const {
  if (auto cmp = allows_address_of <=> rhs.allows_address_of; cmp != 0) return cmp;
  else if (auto cmp = type <=> rhs.type; cmp != 0) return cmp;
  return bits <=> rhs.bits;
}

pepp::debug::TypedBits pepp::debug::with_bits(const TypedBits &type, uint64_t new_value) {
  return {.allows_address_of = type.allows_address_of, .type = type.type, .bits = new_value};
  switch (type.type) {
  case ExpressionType::i8:
    return {.allows_address_of = type.allows_address_of, .type = type.type, .bits = uint64_t((int8_t)new_value)};
  case ExpressionType::u8:
    return {.allows_address_of = type.allows_address_of, .type = type.type, .bits = uint64_t((uint8_t)new_value)};
  case ExpressionType::i16:
    return {.allows_address_of = type.allows_address_of, .type = type.type, .bits = uint64_t((int16_t)new_value)};
  case ExpressionType::u16:
    return {.allows_address_of = type.allows_address_of, .type = type.type, .bits = uint64_t((uint16_t)new_value)};
  case ExpressionType::i32:
    return {.allows_address_of = type.allows_address_of, .type = type.type, .bits = uint64_t((int32_t)new_value)};
  case ExpressionType::u32:
    return {.allows_address_of = type.allows_address_of, .type = type.type, .bits = uint64_t((uint32_t)new_value)};
  }
}

pepp::debug::TypedBits pepp::debug::promote(const TypedBits &value, ExpressionType as_type) {
  if (as_type == value.type) return value;
  switch (as_type) {
  case ExpressionType::i8:
    return {.allows_address_of = value.allows_address_of, .type = as_type, .bits = uint64_t((int8_t)value.bits)};
  case ExpressionType::u8:
    return {.allows_address_of = value.allows_address_of, .type = as_type, .bits = uint64_t((uint8_t)value.bits)};
  case ExpressionType::i16:
    return {.allows_address_of = value.allows_address_of, .type = as_type, .bits = uint64_t((int16_t)value.bits)};
  case ExpressionType::u16:
    return {.allows_address_of = value.allows_address_of, .type = as_type, .bits = uint64_t((uint16_t)value.bits)};
  case ExpressionType::i32:
    return {.allows_address_of = value.allows_address_of, .type = as_type, .bits = uint64_t((int32_t)value.bits)};
  case ExpressionType::u32:
    return {.allows_address_of = value.allows_address_of, .type = as_type, .bits = uint64_t((uint32_t)value.bits)};
  }
}

pepp::debug::TypedBits operator+(const pepp::debug::TypedBits &arg) {
  if (pepp::debug::is_unsigned(arg.type)) return arg;
  else {
    switch (pepp::debug::bitness(arg.type)) {
    case 8: return with_bits(arg, +((int8_t)arg.bits));
    case 16: return with_bits(arg, +((int16_t)arg.bits));
    case 32: return with_bits(arg, +((int32_t)arg.bits));
    }
  }
  throw std::logic_error("Not implemented");
}
pepp::debug::TypedBits operator-(const pepp::debug::TypedBits &arg) {

  switch (pepp::debug::bitness(arg.type)) {
  case 8: return with_bits(arg, -((int8_t)arg.bits));
  case 16: return with_bits(arg, -((int16_t)arg.bits));
  case 32: return with_bits(arg, -((int32_t)arg.bits));
  }
  throw std::logic_error("Not implemented");
}
pepp::debug::TypedBits operator*(const pepp::debug::TypedBits &arg) { throw std::logic_error("Not implemented"); }
pepp::debug::TypedBits operator&(const pepp::debug::TypedBits &arg) { throw std::logic_error("Not implemented"); }
pepp::debug::TypedBits operator!(const pepp::debug::TypedBits &arg) {
  switch (pepp::debug::bitness(arg.type)) {
  case 8: return with_bits(arg, !((int8_t)arg.bits));
  case 16: return with_bits(arg, !((int16_t)arg.bits));
  case 32: return with_bits(arg, !((int32_t)arg.bits));
  }
  throw std::logic_error("Not implemented");
}

pepp::debug::TypedBits operator~(const pepp::debug::TypedBits &arg) {
  switch (pepp::debug::bitness(arg.type)) {
  case 8: return with_bits(arg, ~((uint8_t)arg.bits));
  case 16: return with_bits(arg, ~((uint16_t)arg.bits));
  case 32: return with_bits(arg, ~((uint32_t)arg.bits));
  }
  throw std::logic_error("Not implemented");
}

pepp::debug::TypedBits operator*(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits * rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) > promote(rhs, type);
}

pepp::debug::TypedBits operator/(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits / rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) / promote(rhs, type);
}

pepp::debug::TypedBits operator%(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits % rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) % promote(rhs, type);
}

pepp::debug::TypedBits operator+(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits + rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) + promote(rhs, type);
}

pepp::debug::TypedBits operator-(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits - rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) - promote(rhs, type);
}

pepp::debug::TypedBits operator<<(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits << rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) << promote(rhs, type);
}

pepp::debug::TypedBits operator>>(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits >> rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) >> promote(rhs, type);
}

pepp::debug::TypedBits operator<(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits < rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) < promote(rhs, type);
}

pepp::debug::TypedBits operator<=(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits <= rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) <= promote(rhs, type);
}

pepp::debug::TypedBits operator==(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits == rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) == promote(rhs, type);
}

pepp::debug::TypedBits operator!=(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits != rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) != promote(rhs, type);
}

pepp::debug::TypedBits operator>(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits > rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) > promote(rhs, type);
}

pepp::debug::TypedBits operator>=(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits >= rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) >= promote(rhs, type);
}

pepp::debug::TypedBits operator&(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits & rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) & promote(rhs, type);
}

pepp::debug::TypedBits operator|(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits | rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) | promote(rhs, type);
}

pepp::debug::TypedBits operator^(const pepp::debug::TypedBits &lhs, const pepp::debug::TypedBits &rhs) {
  if (lhs.type == rhs.type) return pepp::debug::with_bits(lhs, lhs.bits ^ rhs.bits);
  auto type = pepp::debug::common_type(lhs.type, rhs.type);
  return promote(lhs, type) ^ promote(rhs, type);
}
