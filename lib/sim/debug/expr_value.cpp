#include "expr_value.hpp"
#include <stdexcept>

std::strong_ordering pepp::debug::VNever::operator<=>(const VNever &other) const { return std::strong_ordering::equal; }

bool pepp::debug::VNever::operator==(const VNever &other) const { return true; }

pepp::debug::VPrimitive pepp::debug::VPrimitive::with_bits(const VPrimitive &type, quint64 new_value) {
  using enum pepp::debug::types::Primitives;
  auto hnd = types::RuntimeTypeInfo::Handle(type.primitive);
  switch (type.primitive) {
  case i8: return VPrimitive{Primitive{.primitive = type.primitive}, hnd, uint64_t((int8_t)new_value)};
  case u8: return VPrimitive{Primitive{.primitive = type.primitive}, hnd, uint64_t((uint8_t)new_value)};
  case i16: return VPrimitive{Primitive{.primitive = type.primitive}, hnd, uint64_t((int16_t)new_value)};
  case u16: return VPrimitive{Primitive{.primitive = type.primitive}, hnd, uint64_t((uint16_t)new_value)};
  case i32: return VPrimitive{Primitive{.primitive = type.primitive}, hnd, uint64_t((int32_t)new_value)};
  case u32: return VPrimitive{Primitive{.primitive = type.primitive}, hnd, uint64_t((uint32_t)new_value)};
  }
  throw std::logic_error("Unreachable");
}

pepp::debug::VPrimitive pepp::debug::VPrimitive::promote(const VPrimitive &value, types::Primitives new_type) {
  using enum pepp::debug::types::Primitives;
  auto hnd = types::RuntimeTypeInfo::Handle(new_type);
  if (new_type == value.primitive) return value;
  return from(new_type, value.bits);
}

pepp::debug::VPrimitive pepp::debug::VPrimitive::from(types::Primitives new_type, quint64 bits) {
  using enum pepp::debug::types::Primitives;
  auto hnd = types::RuntimeTypeInfo::Handle(new_type);
  switch (new_type) {
  case i8: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((int8_t)bits)};
  case u8: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((uint8_t)bits)};
  case i16: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((int16_t)bits)};
  case u16: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((uint16_t)bits)};
  case i32: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((int32_t)bits)};
  case u32: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((uint32_t)bits)};
  }
  throw std::logic_error("Unreachable");
}

pepp::debug::VPrimitive pepp::debug::VPrimitive::True() {
  using enum pepp::debug::types::Primitives;
  static const auto hnd = types::RuntimeTypeInfo::Handle(u8);
  return VPrimitive{Primitive{.primitive = u8}, hnd, 1};
}
pepp::debug::VPrimitive pepp::debug::VPrimitive::False() {
  using enum pepp::debug::types::Primitives;
  static const auto hnd = types::RuntimeTypeInfo::Handle(u8);
  return VPrimitive{Primitive{.primitive = u8}, hnd, 0};
}

pepp::debug::VPrimitive pepp::debug::VPrimitive::i8(int8_t v) {
  using enum pepp::debug::types::Primitives;
  static const auto hnd = types::RuntimeTypeInfo::Handle(u8);
  return VPrimitive{Primitive{.primitive = u8}, hnd, (uint64_t)v};
}

std::strong_ordering pepp::debug::VPrimitive::operator<=>(const VPrimitive &other) const {
  if (auto ret = primitive <=> other.primitive; ret != 0) return ret;
  return bits <=> other.bits;
}

bool pepp::debug::VPrimitive::operator==(const VPrimitive &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::VPointer::operator<=>(const VPointer &other) const {
  if (auto ret = type_handle <=> other.type_handle; ret != 0) return ret;
  return bits <=> other.bits;
}

bool pepp::debug::VPointer::operator==(const VPointer &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::VArray::operator<=>(const VArray &other) const {
  if (auto ret = type_handle <=> other.type_handle; ret != 0) return ret;
  return bits <=> other.bits;
}

bool pepp::debug::VArray::operator==(const VArray &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::VStruct::operator<=>(const VStruct &other) const {
  if (auto ret = type_handle <=> other.type_handle; ret != 0) return ret;
  return bits <=> other.bits;
}

bool pepp::debug::VStruct::operator==(const VStruct &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

namespace detail {
using namespace pepp::debug;
struct OrderingVisitor {
  template <typename T> std::strong_ordering operator()(const T &lhs, const T &rhs) { return lhs <=> rhs; }

  template <typename T, typename U>
    requires(!std::is_same_v<T, U>)
  std::strong_ordering operator()(const T &t, const U &u) {
    return t.type_handle.metatype() <=> u.type_handle.metatype();
  }
};

struct FromBitsVisitor {
  QVariant operator()(const VNever &v) const { return QVariant::fromValue((int8_t)0); }
  QVariant operator()(const VPrimitive &v) const {
    using enum types::Primitives;
    switch (v.primitive) {
    case i8: return QVariant::fromValue((int8_t)v.bits);
    case u8: return QVariant::fromValue((uint8_t)v.bits);
    case i16: return QVariant::fromValue((int16_t)v.bits);
    case u16: return QVariant::fromValue((uint16_t)v.bits);
    case i32: return QVariant::fromValue((int32_t)v.bits);
    case u32: return QVariant::fromValue((uint32_t)v.bits);
    }
    return QVariant::fromValue((int8_t)0);
  }
  // TODO: these actually need RTTI
  QVariant operator()(const VPointer &v) const { return QVariant::fromValue((uint16_t)0); }
  QVariant operator()(const VArray &v) const { return QVariant::fromValue((uint16_t)0); }
  QVariant operator()(const VStruct &v) const { return QVariant::fromValue((uint16_t)0); }
};
} // namespace detail
std::strong_ordering pepp::debug::operator<=>(const Value &lhs, const Value &rhs) {
  return std::visit(::detail::OrderingVisitor{}, lhs, rhs);
}

bool pepp::debug::operator==(const Value &lhs, const Value &rhs) {
  return (lhs <=> rhs) == std::strong_ordering::equal;
}

QVariant pepp::debug::from_bits(const Value &v) { return std::visit(::detail::FromBitsVisitor{}, v); }
