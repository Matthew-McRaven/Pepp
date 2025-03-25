#include "expr_ast.hpp"
#include "expr_tokenizer.hpp"

pepp::debug::Term::~Term() = default;

std::strong_ordering pepp::debug::Variable::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Variable &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Term::Type pepp::debug::Variable::type() const { return Type::Variable; }

uint16_t pepp::debug::Variable::depth() const { return 0; }

std::strong_ordering pepp::debug::Variable::operator<=>(const Variable &rhs) const {
  return std::strong_ordering::equal;
}

QString pepp::debug::Variable::to_string() const { return QStringLiteral("TEST"); }

std::strong_ordering pepp::debug::Constant::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Constant &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Constant::Constant(const detail::UnsignedConstant &constant) { _val = constant; }

uint16_t pepp::debug::Constant::depth() const { return 0; }

pepp::debug::Term::Type pepp::debug::Constant::type() const { return Type::Constant; }

QString pepp::debug::Constant::to_string() const {
  using namespace Qt::Literals::StringLiterals;
  using namespace pepp::debug;
  switch (_val.format) {
  case detail::UnsignedConstant::Format::Dec: return u"%1"_s.arg(_val.value, 0, 10);
  case detail::UnsignedConstant::Format::Hex: return u"0x%1"_s.arg(_val.value, 0, 16);
  }
}

uint16_t pepp::debug::UnaryPrefix::depth() const { return 0; }
pepp::debug::Term::Type pepp::debug::UnaryPrefix::type() const { return Term::Type::UnaryPrefixOperator; }
std::strong_ordering pepp::debug::UnaryPrefix::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const UnaryPrefix &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::UnaryPrefix::operator<=>(const UnaryPrefix &rhs) const {
  if (auto cmp = op <=> rhs.op; cmp != 0) return cmp;
  return *_arg1 <=> *rhs._arg1;
}

QString pepp::debug::UnaryPrefix::to_string() const { return QStringLiteral("OP"); }

uint16_t pepp::debug::BinaryInfix::depth() const { return std::max(arg1->depth(), arg2->depth()) + 1; }
pepp::debug::Term::Type pepp::debug::BinaryInfix::type() const { return Type::BinaryInfixOperator; }
std::strong_ordering pepp::debug::BinaryInfix::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const BinaryInfix &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::BinaryInfix::operator<=>(const BinaryInfix &rhs) const {
  if (auto cmp = op <=> rhs.op; cmp != 0) return cmp;
  else if (auto cmp = *arg1 <=> *rhs.arg1; cmp != 0) return cmp;
  return *arg2 <=> *rhs.arg2;
}

QString pepp::debug::BinaryInfix::to_string() const { return QStringLiteral("OP"); }

std::strong_ordering pepp::debug::Parenthesized::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Parenthesized &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Parenthesized::Parenthesized(std::shared_ptr<Term> term) : _term(term) {}

QString pepp::debug::Parenthesized::to_string() const {
  using namespace Qt::Literals::StringLiterals;
  return u"(%1)"_s.arg(_term->to_string());
}

pepp::debug::Term::Type pepp::debug::Parenthesized::type() const { return Type::ParenExpr; }

uint16_t pepp::debug::Parenthesized::depth() const { return 0; }

std::strong_ordering pepp::debug::Parenthesized::operator<=>(const Parenthesized &rhs) const {
  return std::strong_ordering::equal;
}
