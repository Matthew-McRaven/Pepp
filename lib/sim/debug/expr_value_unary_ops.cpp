#include <stdexcept>
#include "./expr_value.hpp"

namespace detail {
using namespace pepp::debug;

struct TypeofVisitor {
  const types::TypeInfo &info;
  types::Type operator()(const VNever &) const { return types::Never{}; }
  types::Type operator()(const VPrimitive &v) const { return types::Type{v}; }
  types::Type operator()(const VPointer &v) const { return unbox(info.type_from(v.type_handle)); }
  types::Type operator()(const VArray &v) const { return unbox(info.type_from(v.type_handle)); }
  types::Type operator()(const VStruct &v) const { return unbox(info.type_from(v.type_handle)); }
};

struct DerefTypeofVisitor {
  const types::TypeInfo &info;
  types::BoxedType operator()(const types::Never &) const {
    types::Type never = types::Never{};
    auto ret = info.box(never);
    if (!ret.has_value()) throw std::runtime_error("Dereferencing never type");
    return *ret;
  }
  types::BoxedType operator()(const types::Primitive &v) const {
    types::Type i16 = types::Primitive{types::Primitives::i16};
    auto ret = info.box(i16);
    if (!ret.has_value()) throw std::runtime_error("Dereferencing never type");
    return *ret;
  }
  types::BoxedType operator()(const types::Pointer &v) const { return v.to; }
  types::BoxedType operator()(const types::Array &v) const { return v.of; }
  types::BoxedType operator()(const types::Struct &v) const {
    types::Type never = types::Never{};
    auto ret = info.box(never);
    if (!ret.has_value()) throw std::runtime_error("Dereferencing never type");
    return *ret;
  }
};

struct UnaryUnimplementedVisitor {
  Value operator()(const auto &arg) const { return VNever{}; }
};

struct UnaryPlusVisitor {
  Value operator()(const auto &arg) const { return VNever{}; }
  Value operator()(const VPrimitive &arg) {
    if (is_unsigned(arg)) return arg;
    switch (bitness(arg)) {
    case 8: return VPrimitive::with_bits(arg, +((int8_t)arg.bits));
    case 16: return VPrimitive::with_bits(arg, +((int16_t)arg.bits));
    case 32: return VPrimitive::with_bits(arg, +((int32_t)arg.bits));
    }
    return VNever{};
  }
};

struct UnaryMinusVisitor {
  Value operator()(const auto &arg) const { return VNever{}; }
  Value operator()(const VPrimitive &arg) {
    switch (bitness(arg)) {
    case 8: return VPrimitive::with_bits(arg, -((int8_t)arg.bits));
    case 16: return VPrimitive::with_bits(arg, -((int16_t)arg.bits));
    case 32: return VPrimitive::with_bits(arg, -((int32_t)arg.bits));
    }
    return VNever{};
  }
};

struct UnaryNotVisitor {
  const types::TypeInfo &info;
  Value operator()(const VNever &arg) const { return VNever{}; }
  Value operator()(const auto &arg) const {
    using enum types::Primitives;
    auto ret_hnd = info.get_direct(i8);
    if (!ret_hnd) return VNever{};
    auto type = unbox(info.type_from(arg.type_handle));
    switch (bitness(type)) {
    case 8: return VPrimitive{{.primitive = i8}, *ret_hnd, !((int8_t)arg.bits)};
    case 16: return VPrimitive{{.primitive = i8}, *ret_hnd, !((int16_t)arg.bits)};
    case 32: return VPrimitive{{.primitive = i8}, *ret_hnd, !((int32_t)arg.bits)};
    }
    return VNever{};
  }
};

struct UnaryNegateVisitor {
  Value operator()(const auto &arg) const { return VNever{}; }
  Value operator()(const VPrimitive &arg) {
    switch (bitness(arg)) {
    case 8: return VPrimitive::with_bits(arg, ~((int8_t)arg.bits));
    case 16: return VPrimitive::with_bits(arg, ~((int16_t)arg.bits));
    case 32: return VPrimitive::with_bits(arg, ~((int32_t)arg.bits));
    }
    return VNever{};
  }
};
} // namespace detail

pepp::debug::types::Type pepp::debug::operators::op1_typeof(const types::TypeInfo &info, const Value &v) {
  return std::visit(::detail::TypeofVisitor{.info = info}, v);
}

pepp::debug::types::BoxedType pepp::debug::operators::op1_dereference_typeof(const types::TypeInfo &info,
                                                                             const Value &v) {
  auto _typeof = std::visit(::detail::TypeofVisitor{.info = info}, v);
  return std::visit(::detail::DerefTypeofVisitor{.info = info}, _typeof);
}

pepp::debug::Value pepp::debug::operators::op1_plus(const types::TypeInfo &info, const Value &v) {
  return std::visit(::detail::UnaryPlusVisitor{}, v);
}
pepp::debug::Value pepp::debug::operators::op1_minus(const types::TypeInfo &info, const Value &v) {
  return std::visit(::detail::UnaryMinusVisitor{}, v);
}
pepp::debug::Value pepp::debug::operators::op1_not(const types::TypeInfo &info, const Value &v) {
  return std::visit(::detail::UnaryNotVisitor{.info = info}, v);
}
pepp::debug::Value pepp::debug::operators::op1_negate(const types::TypeInfo &info, const Value &v) {
  return std::visit(::detail::UnaryNegateVisitor{}, v);
}
pepp::debug::Value pepp::debug::operators::op1_dereference(const types::TypeInfo &info, const Value &v) {
  return std::visit(::detail::UnaryUnimplementedVisitor{}, v);
}
pepp::debug::Value pepp::debug::operators::op1_addressof(const types::TypeInfo &info, const Value &v) {
  return std::visit(::detail::UnaryUnimplementedVisitor{}, v);
}
