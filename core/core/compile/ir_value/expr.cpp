#include "expr.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/log2.hpp"
#include "fmt/format.h"

namespace {
// Shifting a u64 by 64 is undefined, so a full-width result needs its own mask.
u64 mask_for(u8 size) noexcept { return size >= sizeof(u64) ? ~0ULL : (1ULL << (size * 8)) - 1; }

u64 evaluate_op(pepp::ast::InfixExpression::Op op, pepp::ast::IRValue *ir_lhs, pepp::ast::IRValue *ir_rhs,
                u8 size) noexcept {
  using Op = pepp::ast::InfixExpression::Op;
  if (!ir_lhs || !ir_rhs) return 0;
  const auto lhs = ir_lhs->value_as<u64>();
  const auto rhs = ir_rhs->value_as<u64>();
  switch (op) {
  case Op::Addition: return (lhs + rhs) & mask_for(size);
  case Op::Subtraction: return (lhs - rhs) & mask_for(size);
  default: return 0;
  }
}
} // namespace

pepp::ast::InfixExpression::InfixExpression(Op op, std::shared_ptr<IRValue> lhs, std::shared_ptr<IRValue> rhs,
                                            u8 size)
    : _op(op), _size(size), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

u64 pepp::ast::InfixExpression::serialized_size() const noexcept { return _size; }

// The result is a _size-byte quantity. Report the fewest bytes that hold it as either an unsigned or a signed value,
// e.g., a 200 and -2 both fit where a one-byte operand is required, but 0x0100 does not.
u64 pepp::ast::InfixExpression::minimum_size() const noexcept {
  const u64 value = evaluate();
  i64 extended = static_cast<i64>(value);
  if (_size > 0 && _size < sizeof(u64) && (value & (1ULL << (_size * 8 - 1)))) extended = static_cast<i64>(value | ~mask_for(_size));
  return std::min<u64>(bits::unsigned_bytecount(value), bits::signed_bytecount(extended));
}

u32 pepp::ast::InfixExpression::serialize(bits::span<u8> dest, bits::Order destEndian, u32 max_size) const noexcept {
  const u64 value = evaluate();
  using size_type = bits::span<const u8>::size_type;
  auto size = std::min<size_type>(max_size, serialized_size());
  std::span<const u8> src(reinterpret_cast<const u8 *>(&value), sizeof(value));
  bits::memcpy_endian(dest, destEndian, src, bits::hostOrder());
  return size;
}

std::string pepp::ast::InfixExpression::string() const {
  if (_op == Op::Nil || !_lhs || !_rhs) return "0";
  std::string op;
  switch (_op) {
  case Op::Addition: op = "+"; break;
  case Op::Subtraction: op = "-"; break;
  default: throw std::runtime_error("Unknown operator in InfixExpression::string()");
  };

  return fmt::format("{} {} {}", _lhs->string(), op, _rhs->string());
}

std::string pepp::ast::InfixExpression::raw_string() const { return string(); }

u64 pepp::ast::InfixExpression::evaluate() const noexcept {
  switch (_op) {
  case Op::Nil: return 0;
  case Op::Addition: [[fallthrough]];
  case Op::Subtraction: return evaluate_op(_op, _lhs.get(), _rhs.get(), _size);
  default: throw std::runtime_error("Unknown operator in InfixExpression::evaluate()");
  }
}

bool pepp::ast::contains_symbol(const IRValue &value) noexcept {
  if (dynamic_cast<const Symbolic *>(&value)) return true;
  if (auto *infix = dynamic_cast<const InfixExpression *>(&value))
    return (infix->lhs() && contains_symbol(*infix->lhs())) || (infix->rhs() && contains_symbol(*infix->rhs()));
  return false;
}
