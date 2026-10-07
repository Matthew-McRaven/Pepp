#include "expr.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/log2.hpp"
#include "fmt/format.h"

namespace {
u64 add(pepp::ast::IRValue *ir_lhs, pepp::ast::IRValue *ir_rhs, u8 size) noexcept {
  const u64 MASK = (1ULL << (size * 8)) - 1;
  if (!ir_lhs || !ir_rhs) return 0;
  auto lhs = ir_lhs->value_as<u64>();
  auto rhs = ir_rhs->value_as<u64>();
  return (lhs + rhs) & MASK;
}
} // namespace

pepp::ast::InfixExpression::InfixExpression(Op op, std::shared_ptr<IRValue> lhs, std::shared_ptr<IRValue> rhs,
                                            u8 size)
    : _op(op), _size(size), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

u64 pepp::ast::InfixExpression::serialized_size() const noexcept { return _size; }

u64 pepp::ast::InfixExpression::minimum_size() const noexcept { return _size; }

u32 pepp::ast::InfixExpression::serialize(bits::span<u8> dest, bits::Order destEndian, u32 max_size) const noexcept {
  const u64 value = evaluate();
  using size_type = bits::span<const u8>::size_type;
  auto size = std::min<size_type>(max_size, serialized_size_for(value));
  std::span<const u8> src(reinterpret_cast<const u8 *>(&value), sizeof(value));
  bits::memcpy_endian(dest, destEndian, src, bits::hostOrder());
  return size;
}

std::string pepp::ast::InfixExpression::string() const {
  if (_op == Op::Nil || !_lhs || !_rhs) return "0";
  std::string op;
  switch (_op) {
  case Op::Addition: op = "+"; break;
  default: throw std::runtime_error("Unknown operator in InfixExpression::string()");
  };

  return fmt::format("{} {} {}", _lhs->string(), op, _rhs->string());
}

std::string pepp::ast::InfixExpression::raw_string() const { return string(); }

u64 pepp::ast::InfixExpression::evaluate() const noexcept {
  switch (_op) {
  case Op::Nil: return 0;
  case Op::Addition: return add(_lhs.get(), _rhs.get(), _size);
  default: throw std::runtime_error("Unknown operator in InfixExpression::evaluate()");
  }
}

u64 pepp::ast::InfixExpression::serialized_size_for(u64 value) const noexcept { return bits::signed_bytecount(value); }
