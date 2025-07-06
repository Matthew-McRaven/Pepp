#include "expr_value2.hpp"

pepp::debug::VPrimitive pepp::debug::VPrimitive::with_bits(const VPrimitive &type, quint64 new_value) {
  using enum pepp::debug::types::Primitives;
  auto hnd = types::RuntimeTypeInfo::Handle(type.primitive);
  switch (type.primitive) {
  case i8: return VPrimitive{TPrimitive{.primitive = type.primitive}, hnd, uint64_t((int8_t)new_value)};
  case u8: return VPrimitive{TPrimitive{.primitive = type.primitive}, hnd, uint64_t((uint8_t)new_value)};
  case i16: return VPrimitive{TPrimitive{.primitive = type.primitive}, hnd, uint64_t((int16_t)new_value)};
  case u16: return VPrimitive{TPrimitive{.primitive = type.primitive}, hnd, uint64_t((uint16_t)new_value)};
  case i32: return VPrimitive{TPrimitive{.primitive = type.primitive}, hnd, uint64_t((int32_t)new_value)};
  case u32: return VPrimitive{TPrimitive{.primitive = type.primitive}, hnd, uint64_t((uint32_t)new_value)};
  }
}

pepp::debug::VPrimitive pepp::debug::VPrimitive::promote(const VPrimitive &value, types::Primitives new_type) {
  using enum pepp::debug::types::Primitives;
  auto hnd = types::RuntimeTypeInfo::Handle(new_type);
  if (new_type == value.primitive) return value;
  switch (new_type) {
  case i8: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((int8_t)value.bits)};
  case u8: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((uint8_t)value.bits)};
  case i16: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((int16_t)value.bits)};
  case u16: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((uint16_t)value.bits)};
  case i32: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((int32_t)value.bits)};
  case u32: return VPrimitive{{.primitive = new_type}, hnd, uint64_t((uint32_t)value.bits)};
  }
}

pepp::debug::VPrimitive pepp::debug::VPrimitive::True() {
  using enum pepp::debug::types::Primitives;
  static const auto hnd = types::RuntimeTypeInfo::Handle(u8);
  return VPrimitive{TPrimitive{.primitive = u8}, hnd, 1};
}
pepp::debug::VPrimitive pepp::debug::VPrimitive::False() {
  using enum pepp::debug::types::Primitives;
  static const auto hnd = types::RuntimeTypeInfo::Handle(u8);
  return VPrimitive{TPrimitive{.primitive = u8}, hnd, 0};
}

pepp::debug::VPrimitive pepp::debug::VPrimitive::i8(int8_t v) {
  using enum pepp::debug::types::Primitives;
  static const auto hnd = types::RuntimeTypeInfo::Handle(u8);
  return VPrimitive{TPrimitive{.primitive = u8}, hnd, (uint64_t)v};
}

struct OrderingVisitor {
  template <typename T> std::strong_ordering operator()(const T &lhs, const T &rhs) { return lhs <=> rhs; }

  template <typename T, typename U>
    requires(!std::is_same_v<T, U>)
  std::strong_ordering operator()(const T &t, const U &u) {
    return t.type_handle.metatype() <=> u.type_handle.metatype();
  }
};

std::strong_ordering pepp::debug::operator<=>(const Value &lhs, const Value &rhs) {
  return std::visit(OrderingVisitor{}, lhs, rhs);
}

struct TypeofVisitor {
  const pepp::debug::types::RuntimeTypeInfo &info;
  pepp::debug::types::Type operator()(const pepp::debug::VNever &) const { return pepp::debug::types::TNever{}; }
  pepp::debug::types::Type operator()(const pepp::debug::VPrimitive &v) const { return pepp::debug::types::Type{v}; }
  pepp::debug::types::Type operator()(const pepp::debug::VPointer &v) const { return unbox(info.from(v.type_handle)); }
  pepp::debug::types::Type operator()(const pepp::debug::VArray &v) const { return unbox(info.from(v.type_handle)); }
  pepp::debug::types::Type operator()(const pepp::debug::VStruct &v) const { return unbox(info.from(v.type_handle)); }
};

pepp::debug::types::Type pepp::debug::_typeof(const types::RuntimeTypeInfo &info, const Value &v) {
  return std::visit(TypeofVisitor{.info = info}, v);
}
