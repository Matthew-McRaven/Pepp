#include "expr_ast.hpp"
#include "expr_tokenizer.hpp"

pepp::debug::Term::~Term() = default;

std::strong_ordering pepp::debug::Variable::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Variable &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Term::Type pepp::debug::Variable::type() const { return Type::Variable; }

pepp::debug::Variable::Variable(const detail::Identifier &ident) : name(ident.value) {}

pepp::debug::Variable::Variable(QString name) : name(name) {}

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

pepp::debug::TypedBits pepp::debug::Variable::evaluate(EvaluationMode /*mode*/) {
  throw std::logic_error("Not implemented");
}

int pepp::debug::Variable::cv_qualifiers() const { return CVQualifiers::Volatile; }

void pepp::debug::Variable::mark_dirty() { throw std::logic_error("Not implemented"); }

bool pepp::debug::Variable::dirty() const { throw std::logic_error("Not implemented"); }

void pepp::debug::Variable::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::Variable::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

std::strong_ordering pepp::debug::Constant::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Constant &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Constant::Constant(const TypedBits &bits, detail::UnsignedConstant::Format format_hint)
    : format_hint(format_hint), value(bits) {}

uint16_t pepp::debug::Constant::depth() const { return 0; }

std::strong_ordering pepp::debug::Constant::operator<=>(const Constant &rhs) const { return value <=> rhs.value; }

pepp::debug::Term::Type pepp::debug::Constant::type() const { return Type::Constant; }

QString pepp::debug::Constant::to_string() const {
  using namespace Qt::Literals::StringLiterals;
  using namespace pepp::debug;
  switch (format_hint) {
  case detail::UnsignedConstant::Format::Dec: return u"%1"_s.arg(value.bits, 0, 10);
  case detail::UnsignedConstant::Format::Hex: return u"0x%1"_s.arg(value.bits, 0, 16);
  }
}

void pepp::debug::Constant::link() {
  // No-op; there are no nested terms.
}

pepp::debug::TypedBits pepp::debug::Constant::evaluate(EvaluationMode) { return value; }

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

pepp::debug::UnaryPrefix::UnaryPrefix(Operators op, std::shared_ptr<Term> arg)
    : op(op), arg(arg), _state({.dirty = false, .value = std::nullopt}) {}
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

pepp::debug::TypedBits pepp::debug::UnaryPrefix::evaluate(EvaluationMode mode) {
  if (mode == EvaluationMode::UseCache && !_state.dirty && _state.value.has_value()) return *_state.value;

  auto child_mode = mode_for_child(mode);
  auto v = arg->evaluate(child_mode);
  _state.dirty = false;
  switch (op) {
  case Operators::DEREFERENCE: [[fallthrough]];
  case Operators::ADDRESS_OF: throw std::logic_error("Not implemented");
  case Operators::PLUS: return *(_state.value = +v);
  case Operators::MINUS: return *(_state.value = -v);
  case Operators::NOT: return *(_state.value = !v);
  case Operators::NEGATE: return *(_state.value = ~v);
  }
}

int pepp::debug::UnaryPrefix::cv_qualifiers() const {
  switch (op) {
  case Operators::DEREFERENCE: [[fallthrough]];
  case Operators::ADDRESS_OF: return CVQualifiers::Volatile;
  default: return 0;
  }
}

void pepp::debug::UnaryPrefix::mark_dirty() { _state.dirty = true; }

bool pepp::debug::UnaryPrefix::dirty() const { return _state.dirty; }

void pepp::debug::UnaryPrefix::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::UnaryPrefix::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

std::optional<pepp::debug::UnaryPrefix::Operators> pepp::debug::string_to_unary_prefix(QStringView key) {
  auto result = std::find_if(unops.cbegin(), unops.cend(), [key](const auto &it) { return it.second == key; });
  if (result == unops.cend()) return std::nullopt;
  return result->first;
}

pepp::debug::BinaryInfix::BinaryInfix(Operators op, std::shared_ptr<Term> lhs, std::shared_ptr<Term> rhs)
    : op(op), lhs(lhs), rhs(rhs), _state({.dirty = false, .value = std::nullopt}) {}

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

struct Mul {};

pepp::debug::TypedBits pepp::debug::BinaryInfix::evaluate(EvaluationMode mode) {
  if (mode == EvaluationMode::UseCache && !_state.dirty && _state.value.has_value()) return *_state.value;

  auto child_mode = mode_for_child(mode);
  auto v_lhs = lhs->evaluate(child_mode);
  auto v_rhs = rhs->evaluate(child_mode);
  _state.dirty = false;
  switch (op) {
  case Operators::STAR_DOT: [[fallthrough]];
  case Operators::DOT: throw std::logic_error("Not Implemented");
  case Operators::MULTIPLY: return *(_state.value = v_lhs * v_rhs);
  case Operators::DIVIDE: return *(_state.value = v_lhs / v_rhs);
  case Operators::MODULO: return *(_state.value = v_lhs % v_rhs);
  case Operators::ADD: return *(_state.value = v_lhs + v_rhs);
  case Operators::SUBTRACT: return *(_state.value = v_lhs - v_rhs);
  case Operators::SHIFT_LEFT: return *(_state.value = v_lhs << v_rhs);
  case Operators::SHIFT_RIGHT: return *(_state.value = v_lhs >> v_rhs);
  case Operators::LESS: return *(_state.value = _lt(v_lhs, v_rhs));
  case Operators::LESS_OR_EQUAL: return *(_state.value = _le(v_lhs, v_rhs));
  case Operators::EQUAL: return *(_state.value = _eq(v_lhs, v_rhs));
  case Operators::NOT_EQUAL: return *(_state.value = _ne(v_lhs, v_rhs));
  case Operators::GREATER: return *(_state.value = _gt(v_lhs, v_rhs));
  case Operators::GREATER_OR_EQUAL: return *(_state.value = _ge(v_lhs, v_rhs));
  case Operators::BIT_AND: return *(_state.value = v_lhs & v_rhs);
  case Operators::BIT_OR: return *(_state.value = v_lhs | v_rhs);
  case Operators::BIT_XOR: return *(_state.value = v_lhs ^ v_rhs);
  }
}
int pepp::debug::BinaryInfix::cv_qualifiers() const {
  switch (op) {
  case Operators::STAR_DOT: [[fallthrough]];
  case Operators::DOT: return CVQualifiers::Volatile;
  default: return 0;
  }
}

void pepp::debug::BinaryInfix::mark_dirty() { _state.dirty = true; }

bool pepp::debug::BinaryInfix::dirty() const { return _state.dirty; }

void pepp::debug::BinaryInfix::accept(MutatingTermVisitor &visitor) { visitor.accept(*this); }

void pepp::debug::BinaryInfix::accept(ConstantTermVisitor &visitor) const { visitor.accept(*this); }

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

pepp::debug::TypedBits pepp::debug::Parenthesized::evaluate(EvaluationMode mode) { return term->evaluate(mode); }

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

void pepp::debug::Term::mark_tree_dirty() {
  if (dirty()) return;
  mark_dirty();
  for (const auto &weak : _dependents) {
    if (weak.expired()) continue;
    auto shared = weak.lock();
    shared->mark_tree_dirty();
  }
}
