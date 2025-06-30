#include "expr_types.hpp"

#include <stdexcept>

pepp::debug::types::Type::~Type() = default;

pepp::debug::types::Type::NodeType pepp::debug::types::Pointer::node_type() const { return NodeType::Primitive; }

pepp::debug::types::Pointer::Pointer(std::shared_ptr<Type> value) : value(value) {}

bool pepp::debug::types::Pointer::is_unsigned() const { return true; }

uint32_t pepp::debug::types::Pointer::bitness() const {
  // TODO: handle 32 bit pointer (aka RISC-V)
  return 16;
}

QString pepp::debug::types::Pointer::to_string() const { return ""; }

std::strong_ordering pepp::debug::types::Pointer::operator<=>(const Type &rhs) const {
  if (node_type() == rhs.node_type()) return this->operator<=>(static_cast<const Pointer &>(rhs));
  return node_type() <=> rhs.node_type();
}

std::strong_ordering pepp::debug::types::Pointer::operator<=>(const Pointer &rhs) const { return value <=> rhs.value; }

pepp::debug::types::Type::NodeType pepp::debug::types::Primitive::node_type() const { return NodeType::Primitive; }

pepp::debug::types::Primitive::Primitive(Primitives value) : value(value) {}

bool pepp::debug::types::Primitive::is_unsigned() const {
  using enum pepp::debug::types::Primitives;
  switch (value) {
  case i8: [[fallthrough]];
  case i16: [[fallthrough]];
  case i32: return false;
  case u8: [[fallthrough]];
  case u16: [[fallthrough]];
  case u32: return true;
  }
}

uint32_t pepp::debug::types::Primitive::bitness() const {
  using enum pepp::debug::types::Primitives;
  switch (value) {
  case i8: [[fallthrough]];
  case u8: return 8;
  case i16: [[fallthrough]];
  case u16: return 16;
  case i32: [[fallthrough]];
  case u32: return 32;
  }
}

QString pepp::debug::types::Primitive::to_string() const {
  using enum pepp::debug::types::Primitives;
  switch (value) {
  case i8: return "i8";
  case u8: return "u8";
  case i16: return "i16";
  case u16: return "u16";
  case i32: return "i32";
  case u32: return "u32";
  }
}

std::shared_ptr<pepp::debug::types::Type> pepp::debug::types::Pointer::common_type(const Type &other,
                                                                                   TypeCache &cache) const {
  return nullptr;
}

std::shared_ptr<pepp::debug::types::Type> pepp::debug::types::Primitive::common_type(const Type &other,
                                                                                     TypeCache &cache) const {
  return nullptr;
}

std::shared_ptr<pepp::debug::types::Type> pepp::debug::types::Primitive::common_type(const Primitive &rhs,
                                                                                     TypeCache &cache) const {
  const auto &lhs = *this;
  if (lhs <=> rhs == std::strong_ordering::equal) return cache.add_or_return<const Primitive &, Type>(lhs);
  auto lhs_bitness = lhs.bitness(), rhs_bitness = rhs.bitness();
  auto lhs_unsigned = lhs.is_unsigned(), rhs_unsigned = rhs.is_unsigned();
  // TODO: I think these rules can be simplified to reduce the amount of branching.
  // If both share a sign, pick the larger of the two types.
  if (lhs_unsigned == rhs_unsigned) {
    if (lhs_bitness > rhs_bitness) return cache.add_or_return<const Primitive &, Type>(lhs);
    return cache.add_or_return<const Primitive &, Type>(rhs);
  }
  // If only one is signed, prefer the unsigned type unless the signed type is bigger.
  else if (lhs_unsigned) {
    if (lhs_bitness >= rhs_bitness) return cache.add_or_return<const Primitive &, Type>(lhs);
    return cache.add_or_return<const Primitive &, Type>(rhs);
  } else {
    if (rhs_bitness >= lhs_bitness) return cache.add_or_return<const Primitive &, Type>(rhs);
    return cache.add_or_return<const Primitive &, Type>(lhs);
  }
}

std::strong_ordering pepp::debug::types::Primitive::operator<=>(const Type &rhs) const {
  if (node_type() == rhs.node_type()) return this->operator<=>(static_cast<const Primitive &>(rhs));
  return node_type() <=> rhs.node_type();
}

std::strong_ordering pepp::debug::types::Primitive::operator<=>(const Primitive &rhs) const {
  return value <=> rhs.value;
}

pepp::debug::types::Array::Array(std::shared_ptr<Type> type, quint16 size) : size(size), value(type) {}

pepp::debug::types::Type::NodeType pepp::debug::types::Array::node_type() const { return NodeType::Array; }

bool pepp::debug::types::Array::is_unsigned() const { return value->is_unsigned(); }

uint32_t pepp::debug::types::Array::bitness() const {
  // TODO: IDK how we want this to work?
  // Size of the pointer or size of the items?
  return 0;
}

QString pepp::debug::types::Array::to_string() const { return ""; }

std::shared_ptr<pepp::debug::types::Type> pepp::debug::types::Array::common_type(const Type &other,
                                                                                 TypeCache &cache) const {
  return nullptr;
}

std::shared_ptr<pepp::debug::types::Type> pepp::debug::types::Array::common_type(const Array &other,
                                                                                 TypeCache &cache) const {
  return nullptr;
}

std::strong_ordering pepp::debug::types::Array::operator<=>(const Type &rhs) const {
  if (node_type() == rhs.node_type()) return this->operator<=>(static_cast<const Array &>(rhs));
  return node_type() <=> rhs.node_type();
}

std::strong_ordering pepp::debug::types::Array::operator<=>(const Array &rhs) const {
  if (auto v = value <=> rhs.value; v != std::strong_ordering::equal) return v;
  return size <=> rhs.size;
}

pepp::debug::types::Struct::Struct(std::vector<std::shared_ptr<const Type>> members) : members(members) {}

pepp::debug::types::Type::NodeType pepp::debug::types::Struct::node_type() const { return NodeType::Struct; }

uint32_t pepp::debug::types::Struct::bitness() const { return 0; }

QString pepp::debug::types::Struct::to_string() const { return ""; }

std::shared_ptr<pepp::debug::types::Type> pepp::debug::types::Struct::common_type(const Type &other,
                                                                                  TypeCache &cache) const {
  return nullptr;
}

std::shared_ptr<pepp::debug::types::Type> pepp::debug::types::Struct::common_type(const Struct &other,
                                                                                  TypeCache &cache) const {
  return nullptr;
}

std::strong_ordering pepp::debug::types::Struct::operator<=>(const Type &rhs) const {
  if (node_type() == rhs.node_type()) return this->operator<=>(static_cast<const Struct &>(rhs));
  return node_type() <=> rhs.node_type();
}

std::strong_ordering pepp::debug::types::Struct::operator<=>(const Struct &rhs) const {
  return members <=> rhs.members;
}

std::shared_ptr<pepp::debug::types::Type> pepp::debug::types::Never::common_type(const Type &other,
                                                                                 TypeCache &cache) const {
  return cache.add_or_return(Never{});
}

pepp::debug::types::Type::NodeType pepp::debug::types::Never::node_type() const { return NodeType::Never; }

bool pepp::debug::types::Never::is_unsigned() const { return false; }

uint32_t pepp::debug::types::Never::bitness() const { return 0; }

QString pepp::debug::types::Never::to_string() const { return "(void*)"; }

std::strong_ordering pepp::debug::types::Never::operator<=>(const Type &rhs) const {
  if (node_type() == rhs.node_type()) return this->operator<=>(static_cast<const Never &>(rhs));
  return node_type() <=> rhs.node_type();
}

std::strong_ordering pepp::debug::types::Never::operator<=>(const Never &rhs) const {
  return std::strong_ordering::equal;
}
