#include "./expr_value2.hpp"

struct UnaryUnimplementedVisitor {
  pepp::debug::Value operator()(const auto &arg) const { return pepp::debug::VNever{}; }
};

struct UnaryPlusVisitor {
  pepp::debug::Value operator()(const auto &arg) const { return pepp::debug::VNever{}; }
  pepp::debug::Value operator()(const pepp::debug::VPrimitive &arg) {
    if (is_unsigned(arg)) return arg;
    switch (bitness(arg)) {
    case 8: return pepp::debug::VPrimitive::with_bits(arg, +((int8_t)arg.bits));
    case 16: return pepp::debug::VPrimitive::with_bits(arg, +((int16_t)arg.bits));
    case 32: return pepp::debug::VPrimitive::with_bits(arg, +((int32_t)arg.bits));
    }
    return pepp::debug::VNever{};
  }
};

struct UnaryMinusVisitor {
  pepp::debug::Value operator()(const auto &arg) const { return pepp::debug::VNever{}; }
  pepp::debug::Value operator()(const pepp::debug::VPrimitive &arg) {
    switch (bitness(arg)) {
    case 8: return pepp::debug::VPrimitive::with_bits(arg, -((int8_t)arg.bits));
    case 16: return pepp::debug::VPrimitive::with_bits(arg, -((int16_t)arg.bits));
    case 32: return pepp::debug::VPrimitive::with_bits(arg, -((int32_t)arg.bits));
    }
    return pepp::debug::VNever{};
  }
};

struct UnaryNotVisitor {
  pepp::debug::Value operator()(const pepp::debug::VNever &arg) const { return pepp::debug::VNever{}; }
  pepp::debug::Value operator()(const auto &arg) const {
    using enum pepp::debug::types::Primitives;
    switch (bitness(arg)) {
    case 8: return pepp::debug::VPrimitive{{.primitive = i8}, !((int8_t)arg.bits)};
    case 16: return pepp::debug::VPrimitive{{.primitive = i8}, !((int16_t)arg.bits)};
    case 32: return pepp::debug::VPrimitive{{.primitive = i8}, !((int32_t)arg.bits)};
    }
    return pepp::debug::VNever{};
  }
};

struct UnaryNegateVisitor {
  pepp::debug::Value operator()(const auto &arg) const { return pepp::debug::VNever{}; }
  pepp::debug::Value operator()(const pepp::debug::VPrimitive &arg) {
    switch (bitness(arg)) {
    case 8: return pepp::debug::VPrimitive::with_bits(arg, ~((int8_t)arg.bits));
    case 16: return pepp::debug::VPrimitive::with_bits(arg, ~((int16_t)arg.bits));
    case 32: return pepp::debug::VPrimitive::with_bits(arg, ~((int32_t)arg.bits));
    }
    return pepp::debug::VNever{};
  }
};

pepp::debug::Value pepp::debug::operators::operator+(const Value &v) { return std::visit(UnaryPlusVisitor{}, v); }
pepp::debug::Value pepp::debug::operators::operator-(const Value &v) { return std::visit(UnaryMinusVisitor{}, v); }
pepp::debug::Value pepp::debug::operators::operator!(const Value &v) { return std::visit(UnaryNotVisitor{}, v); }
pepp::debug::Value pepp::debug::operators::operator~(const Value &v) { return std::visit(UnaryNegateVisitor{}, v); }
pepp::debug::Value pepp::debug::operators::operator*(const Value &v) {
  return std::visit(UnaryUnimplementedVisitor{}, v);
}
pepp::debug::Value pepp::debug::operators::operator&(const Value &v) {
  return std::visit(UnaryUnimplementedVisitor{}, v);
}
