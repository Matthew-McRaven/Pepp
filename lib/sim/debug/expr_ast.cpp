#include "expr_ast.hpp"
#include "expr_tokenizer.hpp"

pepp::debug::Term::~Term() = default;

std::strong_ordering pepp::debug::Variable::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Variable &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Term::Type pepp::debug::Variable::type() const { return Type::Variable; }

pepp::debug::Variable::Variable(const detail::Identifier &ident) : _name(ident.value) {}

pepp::debug::Variable::Variable(QString name) : _name(name) {}

uint16_t pepp::debug::Variable::depth() const { return 0; }

std::strong_ordering pepp::debug::Variable::operator<=>(const Variable &rhs) const {
  auto v = _name.compare(rhs._name);
  if (v < 0) return std::strong_ordering::less;
  else if (v == 0) return std::strong_ordering::equal;
  return std::strong_ordering::greater;
}

QString pepp::debug::Variable::to_string() const { return _name; }

std::strong_ordering pepp::debug::Constant::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const Constant &>(rhs));
  return type() <=> rhs.type();
}

pepp::debug::Constant::Constant(const detail::UnsignedConstant &constant) { _val = constant; }

uint16_t pepp::debug::Constant::depth() const { return 0; }

std::strong_ordering pepp::debug::Constant::operator<=>(const Constant &rhs) const { return _val <=> rhs._val; }

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

pepp::debug::BinaryInfix::BinaryInfix(Operators op, std::shared_ptr<Term> arg1, std::shared_ptr<Term> arg2)
    : _op(op), _arg1(arg1), _arg2(arg2) {}

uint16_t pepp::debug::BinaryInfix::depth() const { return std::max(_arg1->depth(), _arg2->depth()) + 1; }
pepp::debug::Term::Type pepp::debug::BinaryInfix::type() const { return Type::BinaryInfixOperator; }
std::strong_ordering pepp::debug::BinaryInfix::operator<=>(const Term &rhs) const {
  if (type() == rhs.type()) return this->operator<=>(static_cast<const BinaryInfix &>(rhs));
  return type() <=> rhs.type();
}

std::strong_ordering pepp::debug::BinaryInfix::operator<=>(const BinaryInfix &rhs) const {
  if (auto cmp = _op <=> rhs._op; cmp != 0) return cmp;
  else if (auto cmp = *_arg1 <=> *rhs._arg1; cmp != 0) return cmp;
  return *_arg2 <=> *rhs._arg2;
}

namespace {
using namespace pepp::debug;
using Operators = BinaryInfix::Operators;
static const auto ops = std::map<Operators, QString>{
    {Operators::DOT, "."},        {Operators::STAR_DOT, "->"},      {Operators::MULTIPLY, "*"},
    {Operators::DIVIDE, "/"},     {Operators::MODULO, "%"},         {Operators::ADD, "+"},
    {Operators::SUBTRACT, "-"},   {Operators::SHIFT_LEFT, "<<"},    {Operators::SHIFT_RIGHT, ">>"},
    {Operators::LESS, "<"},       {Operators::LESS_OR_EQUAL, "<="}, {Operators::EQUAL, "=="},
    {Operators::NOT_EQUAL, "!="}, {Operators::GREATER, ">"},        {Operators::GREATER_OR_EQUAL, ">="},
    {Operators::BIT_AND, "&"},    {Operators::BIT_OR, "|"},         {Operators::BIT_XOR, "^"},
};
static const auto padding = std::map<Operators, bool>{
    {Operators::DOT, false},      {Operators::STAR_DOT, false},     {Operators::MULTIPLY, true},
    {Operators::DIVIDE, true},    {Operators::MODULO, true},        {Operators::ADD, true},
    {Operators::SUBTRACT, true},  {Operators::SHIFT_LEFT, false},   {Operators::SHIFT_RIGHT, false},
    {Operators::LESS, true},      {Operators::LESS_OR_EQUAL, true}, {Operators::EQUAL, true},
    {Operators::NOT_EQUAL, true}, {Operators::GREATER, true},       {Operators::GREATER_OR_EQUAL, true},
    {Operators::BIT_AND, false},  {Operators::BIT_OR, false},       {Operators::BIT_XOR, false},
};
} // namespace

QString pepp::debug::BinaryInfix::to_string() const {
  using namespace Qt::StringLiterals;
  if (padding.at(this->_op)) return u"%1 %2 %3"_s.arg(_arg1->to_string(), ops.at(this->_op), _arg2->to_string());
  return u"%1%2%3"_s.arg(_arg1->to_string(), ops.at(this->_op), _arg2->to_string());
}

std::optional<pepp::debug::BinaryInfix::Operators> pepp::debug::string_to_binary_infix(QStringView key) {
  auto result = std::find_if(ops.cbegin(), ops.cend(), [key](const auto &it) { return it.second == key; });
  if (result == ops.cend()) return std::nullopt;
  return result->first;
}

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
