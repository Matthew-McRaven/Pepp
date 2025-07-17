#include "expr_ast.hpp"
#include <stdexcept>
#include "expr_eval.hpp"
#include "expr_tokenizer.hpp"

pepp::debug::Term::~Term() = default;

pepp::debug::CachedEvaluator pepp::debug::Term::evaluator() { return CachedEvaluator(shared_from_this()); }

std::strong_ordering pepp::debug::Variable::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Variable &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Term::Type pepp::debug::Variable::type() const { return Type::Variable; }

pepp::debug::Variable::Variable(const detail::Identifier &ident) : name(ident.value) {
  _state.set_depends_on_volatiles(true);
}

pepp::debug::Variable::Variable(QString name) : name(name) { _state.set_depends_on_volatiles(true); }

uint16_t pepp::debug::Variable::depth() const { return 0; }

std::strong_ordering pepp::debug::Variable::operator<=>(const Variable &rhs) const {
  if (auto v = name.compare(rhs.name); v < 0) return std::strong_ordering::less;
  else if (v == 0) return std::strong_ordering::equal;
  return std::strong_ordering::greater;
}

QString pepp::debug::Variable::to_string() const { return name; }

void pepp::debug::Variable::link() {
  // No-op; there are no nested terms.
}

pepp::debug::Value pepp::debug::Variable::evaluate(CachePolicy mode, Environment &env) {
  if (_state.value.has_value()) {
    using enum CachePolicy;
    switch (mode) {
    case UseNever: break;
    case UseNonVolatiles: break;
    case UseAlways:
      if (_state.dirty()) break;
    case UseDirtyAlways: return *_state.value;
    }
  }

  auto new_value = env.evaluate_variable(name);
  _state.mark_clean(new_value != _state.value);

  return *(_state.value = new_value);
}

pepp::debug::EvaluationCache pepp::debug::Variable::cached() const { return _state; }

int pepp::debug::Variable::cv_qualifiers() const { return CVQualifiers::Volatile; }

void pepp::debug::Variable::mark_dirty() { _state.mark_dirty(); }

bool pepp::debug::Variable::dirty() const { return _state.dirty(); }

void pepp::debug::Variable::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::Variable::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

std::strong_ordering pepp::debug::Constant::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Constant &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Constant::Constant(const VPrimitive &bits, detail::UnsignedConstant::Format format_hint)
    : format_hint(format_hint), value(bits) {}

uint16_t pepp::debug::Constant::depth() const { return 0; }

std::strong_ordering pepp::debug::Constant::operator<=>(const Constant &rhs) const { return value <=> rhs.value; }

pepp::debug::Term::Type pepp::debug::Constant::type() const { return Type::Constant; }

QString pepp::debug::Constant::to_string() const {
  using namespace Qt::Literals::StringLiterals;
  using namespace pepp::debug;
  using enum pepp::debug::types::Primitives;
  switch (format_hint) {
  case detail::UnsignedConstant::Format::Dec:
    switch (value.primitive) {
    case i8: return u"%1_i8"_s.arg((int8_t)value.bits, 0, 10);
    case u8: return u"%1_u8"_s.arg((uint8_t)value.bits, 0, 10);
    case i16: return u"%1"_s.arg((int16_t)value.bits, 0, 10);
    case u16: return u"%1_u16"_s.arg((uint16_t)value.bits, 0, 10);
    case i32: return u"%1_i32"_s.arg((int32_t)value.bits, 0, 10);
    case u32: return u"%1_u32"_s.arg((uint32_t)value.bits, 0, 10);
    }
  case detail::UnsignedConstant::Format::Hex:
    switch (value.primitive) {
    case i8: return u"0x%1_i8"_s.arg((uint8_t)value.bits, 0, 16);
    case u8: return u"0x%1_u8"_s.arg((uint8_t)value.bits, 0, 16);
    case i16: return u"0x%1"_s.arg((uint16_t)value.bits, 0, 16);
    case u16: return u"0x%1_u16"_s.arg((uint16_t)value.bits, 0, 16);
    case i32: return u"0x%1_i32"_s.arg((uint32_t)value.bits, 0, 16);
    case u32: return u"0x%1_u32"_s.arg((uint32_t)value.bits, 0, 16);
    }
  }
  throw std::logic_error("Unreachable");
}

void pepp::debug::Constant::link() {
  // No-op; there are no nested terms.
}

pepp::debug::Value pepp::debug::Constant::evaluate(CachePolicy, Environment &) { return value; }

pepp::debug::EvaluationCache pepp::debug::Constant::cached() const { return EvaluationCache(value); }

int pepp::debug::Constant::cv_qualifiers() const { return CVQualifiers::Constant; }

void pepp::debug::Constant::mark_dirty() {}

bool pepp::debug::Constant::dirty() const { return false; }

void pepp::debug::Constant::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::Constant::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

namespace {
using UOperators = pepp::debug::UnaryPrefix::Operators;
const auto unops = std::map<UOperators, QString>{
    {UOperators::PLUS, "+"},       {UOperators::MINUS, "-"}, {UOperators::DEREFERENCE, "*"},
    {UOperators::ADDRESS_OF, "&"}, {UOperators::NOT, "!"},   {UOperators::NEGATE, "~"},
};
} // namespace

pepp::debug::UnaryPrefix::UnaryPrefix(Operators op, std::shared_ptr<Term> arg) : op(op), arg(arg) {
  int arg_cv = arg ? arg->cv_qualifiers() : 0;
  bool is_volatile = (arg_cv | UnaryPrefix::cv_qualifiers()) & CVQualifiers::Volatile;
  _state.set_depends_on_volatiles(is_volatile);
}
uint16_t pepp::debug::UnaryPrefix::depth() const { return arg->depth() + 1; }
pepp::debug::Term::Type pepp::debug::UnaryPrefix::type() const { return Term::Type::UnaryPrefixOperator; }

std::strong_ordering pepp::debug::UnaryPrefix::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const UnaryPrefix &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::UnaryPrefix::operator<=>(const UnaryPrefix &rhs) const {
  if (auto cmp = op <=> rhs.op; cmp != 0) return cmp;
  return *arg <=> *rhs.arg;
}

QString pepp::debug::UnaryPrefix::to_string() const {
  using namespace Qt::StringLiterals;
  return u"%1%2"_s.arg(unops.at(this->op), arg->to_string());
}

void pepp::debug::UnaryPrefix::link() { arg->add_dependent(weak_from_this()); }

pepp::debug::Value pepp::debug::UnaryPrefix::evaluate(CachePolicy mode, Environment &env) {
  using namespace pepp::debug::operators;
  if (_state.value.has_value()) {
    using enum CachePolicy;
    switch (mode) {
    case UseNever: break;
    case UseNonVolatiles:
      if (_state.depends_on_volatiles()) break;
    case UseAlways:
      if (_state.dirty()) break;
    case UseDirtyAlways: return *_state.value;
    }
  }
  auto rtti = env.type_info();
  auto eval = arg->evaluator();
  auto v = eval.evaluate(mode, env);
  _state.mark_clean();
  switch (op) {
  case Operators::DEREFERENCE: throw std::logic_error("Use MemoryAccess instead");
  case Operators::ADDRESS_OF: throw std::logic_error("Not implemented");
  case Operators::PLUS: return *(_state.value = op1_plus(*rtti, v));
  case Operators::MINUS: return *(_state.value = op1_minus(*rtti, v));
  case Operators::NOT: return *(_state.value = op1_not(*rtti, v));
  case Operators::NEGATE: return *(_state.value = op1_negate(*rtti, v));
  }
  throw std::logic_error("Unimplemented");
}

pepp::debug::EvaluationCache pepp::debug::UnaryPrefix::cached() const { return _state; }

int pepp::debug::UnaryPrefix::cv_qualifiers() const {
  switch (op) {
  case Operators::DEREFERENCE: [[fallthrough]];
  case Operators::ADDRESS_OF: return CVQualifiers::Volatile;
  default: return 0;
  }
}

void pepp::debug::UnaryPrefix::mark_dirty() { _state.mark_dirty(); }

bool pepp::debug::UnaryPrefix::dirty() const { return _state.dirty(); }

void pepp::debug::UnaryPrefix::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::UnaryPrefix::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

std::optional<pepp::debug::UnaryPrefix::Operators> pepp::debug::string_to_unary_prefix(QStringView key) {
  auto result = std::find_if(unops.cbegin(), unops.cend(), [key](const auto &it) { return it.second == key; });
  if (result == unops.cend()) return std::nullopt;
  return result->first;
}

pepp::debug::MemoryRead::MemoryRead(std::shared_ptr<Term> arg) : arg(arg) { _state.set_depends_on_volatiles(true); }

std::strong_ordering pepp::debug::MemoryRead::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const MemoryRead &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::MemoryRead::operator<=>(const MemoryRead &rhs) const { return *arg <=> *rhs.arg; }

uint16_t pepp::debug::MemoryRead::depth() const { return arg->depth() + 1; }

pepp::debug::Term::Type pepp::debug::MemoryRead::type() const { return Term::Type::MemoryRead; }

QString pepp::debug::MemoryRead::to_string() const {
  using namespace Qt::StringLiterals;
  return u"*%1"_s.arg(arg->to_string());
}

void pepp::debug::MemoryRead::link() { arg->add_dependent(weak_from_this()); }

int pepp::debug::MemoryRead::cv_qualifiers() const { return CVQualifiers::Volatile; }

void pepp::debug::MemoryRead::mark_dirty() { _state.mark_dirty(); }

bool pepp::debug::MemoryRead::dirty() const { return _state.dirty(); }

void pepp::debug::MemoryRead::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::MemoryRead::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

pepp::debug::Value pepp::debug::MemoryRead::evaluate(CachePolicy mode, Environment &env) {
  using namespace pepp::debug::operators;
  if (_state.value.has_value()) {
    using enum CachePolicy;
    switch (mode) {
    case UseNever: break;
    case UseNonVolatiles: break;
    case UseAlways:
      if (_state.dirty()) break;
    case UseDirtyAlways: return *_state.value;
    }
  }

  auto rtti = env.type_info();
  auto eval = arg->evaluator();
  auto v = eval.evaluate(mode, env);
  auto ret_type = operators::op1_dereference_typeof(*rtti, v);
  auto address = value_bits(v);
  auto bytecount = bitness(types::unbox(ret_type)) / 8;
  quint64 readBuf = 0;
  switch (bytecount) {
  case 1: readBuf = env.read_mem_u8(address); break;
  case 4: readBuf |= (quint32)(env.read_mem_u16(address += 2) << 16); [[fallthrough]];
  case 2: readBuf |= env.read_mem_u16(address); break;
  default: throw std::logic_error("MemoryRead: Unsupported size");
  }

  _state.mark_clean();

  return *(_state.value = from_bits(unbox(ret_type), readBuf));
}

pepp::debug::EvaluationCache pepp::debug::MemoryRead::cached() const { return _state; }

pepp::debug::BinaryInfix::BinaryInfix(Operators op, std::shared_ptr<Term> lhs, std::shared_ptr<Term> rhs)
    : op(op), lhs(lhs), rhs(rhs) {
  switch (op) {
  case Operators::STAR_DOT: [[fallthrough]];
  case Operators::DOT: throw std::logic_error("Use MemberAccess for member access");
  default: break;
  }
  int lhs_cv = lhs ? lhs->cv_qualifiers() : 0;
  int rhs_cv = rhs ? rhs->cv_qualifiers() : 0;
  bool is_volatile = (lhs_cv | rhs_cv) & CVQualifiers::Volatile;
  _state.set_depends_on_volatiles(is_volatile);
}

uint16_t pepp::debug::BinaryInfix::depth() const { return std::max(lhs->depth(), rhs->depth()) + 1; }
pepp::debug::Term::Type pepp::debug::BinaryInfix::type() const { return Type::BinaryInfixOperator; }
std::strong_ordering pepp::debug::BinaryInfix::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const BinaryInfix &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::BinaryInfix::operator<=>(const BinaryInfix &rhs) const {
  if (auto cmp = op <=> rhs.op; cmp != 0) return cmp;
  else if (auto cmp = *this->lhs <=> *rhs.lhs; cmp != 0) return cmp;
  return *this->rhs <=> *rhs.rhs;
}

namespace {
using Operators = pepp::debug::BinaryInfix::Operators;
const auto ops = std::map<Operators, QString>{
    {Operators::DOT, "."},        {Operators::STAR_DOT, "->"},      {Operators::MULTIPLY, "*"},
    {Operators::DIVIDE, "/"},     {Operators::MODULO, "%"},         {Operators::ADD, "+"},
    {Operators::SUBTRACT, "-"},   {Operators::SHIFT_LEFT, "<<"},    {Operators::SHIFT_RIGHT, ">>"},
    {Operators::LESS, "<"},       {Operators::LESS_OR_EQUAL, "<="}, {Operators::EQUAL, "=="},
    {Operators::NOT_EQUAL, "!="}, {Operators::GREATER, ">"},        {Operators::GREATER_OR_EQUAL, ">="},
    {Operators::BIT_AND, "&"},    {Operators::BIT_OR, "|"},         {Operators::BIT_XOR, "^"},
};
const auto padding = std::map<Operators, bool>{
    {Operators::DOT, false},      {Operators::STAR_DOT, false},     {Operators::MULTIPLY, true},
    {Operators::DIVIDE, true},    {Operators::MODULO, true},        {Operators::ADD, true},
    {Operators::SUBTRACT, true},  {Operators::SHIFT_LEFT, false},   {Operators::SHIFT_RIGHT, false},
    {Operators::LESS, true},      {Operators::LESS_OR_EQUAL, true}, {Operators::EQUAL, true},
    {Operators::NOT_EQUAL, true}, {Operators::GREATER, true},       {Operators::GREATER_OR_EQUAL, true},
    {Operators::BIT_AND, true},   {Operators::BIT_OR, true},        {Operators::BIT_XOR, true},
};
} // namespace

QString pepp::debug::BinaryInfix::to_string() const {
  using namespace Qt::StringLiterals;
  if (padding.at(this->op)) return u"%1 %2 %3"_s.arg(lhs->to_string(), ops.at(this->op), rhs->to_string());
  return u"%1%2%3"_s.arg(lhs->to_string(), ops.at(this->op), rhs->to_string());
}

void pepp::debug::BinaryInfix::link() {
  auto weak = weak_from_this();
  lhs->add_dependent(weak);
  rhs->add_dependent(weak);
}

pepp::debug::Value pepp::debug::BinaryInfix::evaluate(CachePolicy mode, Environment &env) {
  using namespace pepp::debug::operators;
  if (_state.value.has_value()) {
    using enum CachePolicy;
    switch (mode) {
    case UseNever: break;
    case UseNonVolatiles:
      if (_state.depends_on_volatiles()) break;
    case UseAlways:
      if (_state.dirty()) break;
    case UseDirtyAlways: return *_state.value;
    }
  }
  auto rtti = env.type_info();
  auto eval_lhs = lhs->evaluator(), eval_rhs = rhs->evaluator();
  auto v_lhs = eval_lhs.evaluate(mode, env), v_rhs = eval_rhs.evaluate(mode, env);
  _state.mark_clean();
  switch (op) {
  case Operators::STAR_DOT: [[fallthrough]];
  case Operators::DOT: throw std::logic_error("Not Implemented");
  case Operators::MULTIPLY: return *(_state.value = op2_mul(*rtti, v_lhs, v_rhs));
  case Operators::DIVIDE: return *(_state.value = op2_div(*rtti, v_lhs, v_rhs));
  case Operators::MODULO: return *(_state.value = op2_mod(*rtti, v_lhs, v_rhs));
  case Operators::ADD: return *(_state.value = op2_add(*rtti, v_lhs, v_rhs));
  case Operators::SUBTRACT: return *(_state.value = op2_sub(*rtti, v_lhs, v_rhs));
  case Operators::SHIFT_LEFT: return *(_state.value = op2_bsl(*rtti, v_lhs, v_rhs));
  case Operators::SHIFT_RIGHT: return *(_state.value = op2_bsr(*rtti, v_lhs, v_rhs));
  case Operators::LESS: return *(_state.value = op2_lt(*rtti, v_lhs, v_rhs));
  case Operators::LESS_OR_EQUAL: return *(_state.value = op2_le(*rtti, v_lhs, v_rhs));
  case Operators::EQUAL: return *(_state.value = op2_eq(*rtti, v_lhs, v_rhs));
  case Operators::NOT_EQUAL: return *(_state.value = op2_ne(*rtti, v_lhs, v_rhs));
  case Operators::GREATER: return *(_state.value = op2_gt(*rtti, v_lhs, v_rhs));
  case Operators::GREATER_OR_EQUAL: return *(_state.value = op2_ge(*rtti, v_lhs, v_rhs));
  case Operators::BIT_AND: return *(_state.value = op2_bitand(*rtti, v_lhs, v_rhs));
  case Operators::BIT_OR: return *(_state.value = op2_bitor(*rtti, v_lhs, v_rhs));
  case Operators::BIT_XOR: return *(_state.value = op2_bitxor(*rtti, v_lhs, v_rhs));
  }
  throw std::logic_error("Unimplemented");
}

pepp::debug::EvaluationCache pepp::debug::BinaryInfix::cached() const { return _state; }

int pepp::debug::BinaryInfix::cv_qualifiers() const { return 0; }

void pepp::debug::BinaryInfix::mark_dirty() { _state.mark_dirty(); }

bool pepp::debug::BinaryInfix::dirty() const { return _state.dirty(); }

void pepp::debug::BinaryInfix::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::BinaryInfix::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

pepp::debug::MemberAccess::MemberAccess(BinaryInfix::Operators op, std::shared_ptr<Term> lhs, QString rhs)
    : op(op), lhs(lhs), rhs(rhs) {
  _state.set_depends_on_volatiles(true);
  switch (op) {
  case BinaryInfix::Operators::DOT:
  case BinaryInfix::Operators::STAR_DOT: break;
  default: throw std::logic_error("Invalid operator for member access");
  }
}

uint16_t pepp::debug::MemberAccess::depth() const { return lhs->depth() + 1; }
pepp::debug::Term::Type pepp::debug::MemberAccess::type() const { return Type::MemberAccess; }
std::strong_ordering pepp::debug::MemberAccess::operator<=>(const Term &other) const {
  if (type() == other.type()) return this->operator<=>(static_cast<const MemberAccess &>(other));
  return type() <=> other.type();
}

std::strong_ordering pepp::debug::MemberAccess::operator<=>(const MemberAccess &other) const {
  if (auto cmp = op <=> other.op; cmp != 0) return cmp;
  else if (cmp = *this->lhs <=> *other.lhs; cmp != 0) return cmp;
  else if (auto str_cmp = this->rhs.compare(other.rhs); str_cmp < 0) return std::strong_ordering::less;
  else if (str_cmp == 0) return std::strong_ordering::equal;
  else return std::strong_ordering::greater;
}

QString pepp::debug::MemberAccess::to_string() const {
  using namespace Qt::StringLiterals;
  if (padding.at(this->op)) return u"%1 %2 %3"_s.arg(lhs->to_string(), ops.at(this->op), rhs);
  return u"%1%2%3"_s.arg(lhs->to_string(), ops.at(this->op), rhs);
}

void pepp::debug::MemberAccess::link() {
  auto weak = weak_from_this();
  lhs->add_dependent(weak);
}

namespace {
struct MemberAccessVisitor {
  quint64 v;
  pepp::debug::Environment &env;
  pepp::debug::Value operator()(const std::shared_ptr<pepp::debug::types::Never> &type) const {
    return pepp::debug::VNever{};
  }
  pepp::debug::Value operator()(const std::shared_ptr<pepp::debug::types::Primitive> &type) const {
    auto info = env.type_info();
    pepp::debug::types::Type var_type = pepp::debug::types::Pointer{2, type};
    auto hnd = info->register_direct(var_type);
    return pepp::debug::VPointer{hnd, v};
  }
  pepp::debug::Value operator()(const std::shared_ptr<pepp::debug::types::Pointer> &type) {
    auto info = env.type_info();
    pepp::debug::types::Type var_type = pepp::debug::types::Pointer{2, type};
    auto hnd = info->register_direct(var_type);
    return pepp::debug::VPointer{hnd, v};
  }
  pepp::debug::Value operator()(const std::shared_ptr<pepp::debug::types::Array> &type) {
    auto info = env.type_info();
    pepp::debug::types::Type var_type = pepp::debug::types::Pointer{2, type};
    auto hnd = info->register_direct(var_type);
    return pepp::debug::VPointer{hnd, v};
  }
  pepp::debug::Value operator()(const std::shared_ptr<pepp::debug::types::Struct> &type) {
    auto info = env.type_info();
    pepp::debug::types::Type var_type = pepp::debug::types::Pointer{2, type};
    auto hnd = info->register_direct(var_type);
    return pepp::debug::VPointer{hnd, v};
  }
};
} // namespace

pepp::debug::Value pepp::debug::MemberAccess::evaluate(CachePolicy mode, Environment &env) {
  using namespace pepp::debug::operators;
  if (_state.value.has_value()) {
    using enum CachePolicy;
    switch (mode) {
    case UseNever: break;
    case UseNonVolatiles:
      if (_state.depends_on_volatiles()) break;
    case UseAlways:
      if (_state.dirty()) break;
    case UseDirtyAlways: return *_state.value;
    }
  }

  switch (op) {
  case Operators::STAR_DOT: [[fallthrough]];
  case Operators::DOT: break;
  default: throw std::logic_error("Use BinaryInfix for non-member-acces binary operators");
  }

  _state.mark_clean();

  auto rtti = env.type_info();
  auto lhs_eval = lhs->evaluator();
  auto lhs_value = lhs_eval.evaluate(mode, env);
  // Confirm that LHS is a actually a struct and that RHS is a member of that struct
  auto lhs_type = op1_typeof(*rtti, lhs_value);
  if (!std::holds_alternative<types::Struct>(lhs_type)) return *(_state.value = VNever{});
  auto lhs_struct = std::get<types::Struct>(lhs_type);
  auto maybe_rhs = lhs_struct.find(rhs);
  if (!maybe_rhs.has_value()) return *(_state.value = VNever{});
  auto [rhs_type, rhs_offset] = *maybe_rhs;
  auto lhs_bits = value_bits(lhs_value);

  // Create pointer to RHS member, return it as a pointer to RHS's type
  auto rhs_ptr = lhs_bits + rhs_offset;
  auto rhs = std::visit(MemberAccessVisitor{rhs_ptr, env}, rhs_type);
  return *(_state.value = rhs);
}

pepp::debug::EvaluationCache pepp::debug::MemberAccess::cached() const { return _state; }

int pepp::debug::MemberAccess::cv_qualifiers() const { return CVQualifiers::Volatile; }

void pepp::debug::MemberAccess::mark_dirty() { _state.mark_dirty(); }

bool pepp::debug::MemberAccess::dirty() const { return _state.dirty(); }

void pepp::debug::MemberAccess::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::MemberAccess::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

std::optional<pepp::debug::BinaryInfix::Operators> pepp::debug::string_to_binary_infix(QStringView key) {
  auto result = std::find_if(ops.cbegin(), ops.cend(), [key](const auto &it) { return it.second == key; });
  if (result == ops.cend()) return std::nullopt;
  return result->first;
}

std::strong_ordering pepp::debug::Parenthesized::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Parenthesized &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Parenthesized::Parenthesized(std::shared_ptr<Term> term) : term(term) {}

QString pepp::debug::Parenthesized::to_string() const {
  using namespace Qt::Literals::StringLiterals;
  return u"(%1)"_s.arg(term->to_string());
}

void pepp::debug::Parenthesized::link() { term->add_dependent(weak_from_this()); }

int pepp::debug::Parenthesized::cv_qualifiers() const { return 0; }

pepp::debug::Value pepp::debug::Parenthesized::evaluate(CachePolicy mode, Environment &env) {
  auto eval = term->evaluator();
  auto v = eval.evaluate(mode, env);
  _state = eval.cache();
  return v;
}

pepp::debug::EvaluationCache pepp::debug::Parenthesized::cached() const { return _state; }

void pepp::debug::Parenthesized::mark_dirty() { term->mark_dirty(); }

bool pepp::debug::Parenthesized::dirty() const { return term->dirty(); }

void pepp::debug::Parenthesized::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::Parenthesized::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

pepp::debug::Term::Type pepp::debug::Parenthesized::type() const { return Type::ParenExpr; }

uint16_t pepp::debug::Parenthesized::depth() const { return term->depth(); }

std::strong_ordering pepp::debug::Parenthesized::operator<=>(const Parenthesized &rhs) const {
  return *term <=> *rhs.term;
}

void pepp::debug::Term::add_dependent(std::weak_ptr<Term> t) { _dependents.emplace_back(t); }

bool pepp::debug::Term::dependency_of(std::shared_ptr<Term> t) const {
  auto compare = [t](const std::weak_ptr<Term> &other) {
    if (other.expired()) return false;
    return other.lock() == t;
  };
  auto it = std::find_if(_dependents.cbegin(), _dependents.cend(), compare);
  return it != _dependents.cend();
}

bits::span<const std::weak_ptr<pepp::debug::Term>> pepp::debug::Term::dependents() const { return _dependents; }

pepp::debug::DirectCast::DirectCast(types::BoxedType cast_to, std::shared_ptr<Term> arg) : _cast_to(cast_to), arg(arg) {
  int arg_cv = arg ? arg->cv_qualifiers() : 0;
  auto is_volatile = arg_cv & CVQualifiers::Volatile;
  _state.set_depends_on_volatiles(is_volatile);
}

std::strong_ordering pepp::debug::DirectCast::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const DirectCast &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::DirectCast::operator<=>(const DirectCast &rhs) const {
  if (auto cmp = _cast_to <=> rhs._cast_to; cmp != 0) return cmp;
  return *arg <=> *rhs.arg;
}

uint16_t pepp::debug::DirectCast::depth() const { return arg->depth() + 1; }

pepp::debug::Term::Type pepp::debug::DirectCast::type() const { return Type::DirectTypeCast; }

QString pepp::debug::DirectCast::to_string() const {
  using namespace Qt::StringLiterals;
  auto u = types::unbox(_cast_to);
  return u"(%1)%2"_s.arg(pepp::debug::types::to_string(u), arg->to_string());
}

void pepp::debug::DirectCast::link() { arg->add_dependent(weak_from_this()); }

pepp::debug::Value pepp::debug::DirectCast::evaluate(CachePolicy mode, Environment &env) {
  if (_state.value.has_value()) {
    using enum CachePolicy;
    switch (mode) {
    case UseNever: break;
    case UseNonVolatiles: break;
    case UseAlways:
      if (_state.dirty()) break;
    case UseDirtyAlways: return *_state.value;
    }
  }

  auto eval = arg->evaluator();
  auto v = eval.evaluate(mode, env);
  _state.mark_clean();
  return *(_state.value = operators::op2_typecast(*env.type_info(), v, this->_cast_to));
}

pepp::debug::EvaluationCache pepp::debug::DirectCast::cached() const { return _state; }

int pepp::debug::DirectCast::cv_qualifiers() const { return 0; }

void pepp::debug::DirectCast::mark_dirty() { _state.mark_dirty(); }

bool pepp::debug::DirectCast::dirty() const { return _state.dirty(); }

void pepp::debug::DirectCast::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::DirectCast::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

const pepp::debug::types::Type pepp::debug::DirectCast::cast_to() const { return types::unbox(_cast_to); }

pepp::debug::IndirectCast::IndirectCast(QString name, types::TypeInfo::IndirectHandle cast_to,
                                        std::shared_ptr<Term> arg)
    : _name(name), _hnd(cast_to), arg(arg) {
  _state.set_depends_on_volatiles(true);
}

std::strong_ordering pepp::debug::IndirectCast::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const IndirectCast &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::IndirectCast::operator<=>(const IndirectCast &rhs) const {
  // Do not compare actual type, since that is a value derived from (handle, version).
  if (auto cmp = _hnd <=> rhs._hnd; cmp != 0) return cmp;
  else if (cmp = _cast_to.version <=> rhs._cast_to.version; cmp != 0) return cmp;
  return *arg <=> *rhs.arg;
}

uint16_t pepp::debug::IndirectCast::depth() const { return arg->depth() + 1; }

pepp::debug::Term::Type pepp::debug::IndirectCast::type() const { return Type::DirectTypeCast; }

QString pepp::debug::IndirectCast::to_string() const {
  using namespace Qt::StringLiterals;
  return u"(%1*)%2"_s.arg(_name, arg->to_string());
}

void pepp::debug::IndirectCast::link() { arg->add_dependent(weak_from_this()); }

pepp::debug::Value pepp::debug::IndirectCast::evaluate(CachePolicy mode, Environment &env) {
  auto versioned_type = env.type_info()->versioned_from(_hnd);
  if (_state.value.has_value()) {
    using enum CachePolicy;
    switch (mode) {
    case UseNever: break;
    case UseNonVolatiles: break;
    case UseAlways:
      if (_state.dirty() || _cast_to.version != versioned_type.version) break;
    case UseDirtyAlways: return *_state.value;
    }
  }
  if (versioned_type.version != _cast_to.version) _cast_to = versioned_type;

  auto eval = arg->evaluator();
  auto v = eval.evaluate(mode, env);
  _state.mark_clean();
  return *(_state.value = operators::op2_typecast(*env.type_info(), v, versioned_type.type));
}

pepp::debug::EvaluationCache pepp::debug::IndirectCast::cached() const { return _state; }

int pepp::debug::IndirectCast::cv_qualifiers() const { return 0; }

void pepp::debug::IndirectCast::mark_dirty() { _state.mark_dirty(); }

bool pepp::debug::IndirectCast::dirty() const { return _state.dirty(); }

void pepp::debug::IndirectCast::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::IndirectCast::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

const pepp::debug::types::Type pepp::debug::IndirectCast::cast_to(const types::TypeInfo &nti) const {
  return types::unbox(nti.type_from(_hnd));
}

const pepp::debug::types::Type pepp::debug::IndirectCast::cast_to(Environment &env) const {
  return cast_to(*env.type_info());
}

pepp::debug::DebuggerVariable::DebuggerVariable(const detail::DebugIdentifier &ident) : name(ident.value) {
  _state.set_depends_on_volatiles(true);
}

pepp::debug::DebuggerVariable::DebuggerVariable(QString name) : name(name) { _state.set_depends_on_volatiles(true); }

uint16_t pepp::debug::DebuggerVariable::depth() const { return 0; }

pepp::debug::Term::Type pepp::debug::DebuggerVariable::type() const { return Type::DebuggerVariable; }

std::strong_ordering pepp::debug::DebuggerVariable::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const DebuggerVariable &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::DebuggerVariable::operator<=>(const DebuggerVariable &rhs) const {
  if (auto v = name.compare(rhs.name); v < 0) return std::strong_ordering::less;
  else if (v == 0) return std::strong_ordering::equal;
  return std::strong_ordering::greater;
}

QString pepp::debug::DebuggerVariable::to_string() const {
  using namespace Qt::StringLiterals;
  return u"$%1"_s.arg(name);
}

void pepp::debug::DebuggerVariable::link() {
  // No-op; there are no nested terms.
}

pepp::debug::Value pepp::debug::DebuggerVariable::evaluate(CachePolicy mode, Environment &env) {
  if (!_name_cache_id.has_value()) _name_cache_id = env.cache_debug_variable_name(name);
  else if (_state.value.has_value()) {
    using enum CachePolicy;
    switch (mode) {
    case UseNever: break;
    case UseNonVolatiles: break;
    case UseAlways:
      if (_state.dirty()) break;
    case UseDirtyAlways: return *_state.value;
    }
  }

  _state.mark_clean();
  return *(_state.value = env.evaluate_debug_variable(*_name_cache_id));
}

pepp::debug::EvaluationCache pepp::debug::DebuggerVariable::cached() const { return _state; }

int pepp::debug::DebuggerVariable::cv_qualifiers() const { return CVQualifiers::Volatile; }

void pepp::debug::DebuggerVariable::mark_dirty() { _state.mark_dirty(); }

bool pepp::debug::DebuggerVariable::dirty() const { return _state.dirty(); }

void pepp::debug::DebuggerVariable::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::DebuggerVariable::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

