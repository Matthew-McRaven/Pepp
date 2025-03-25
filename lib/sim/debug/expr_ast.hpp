#pragma once
#include <QtCore>
#include <memory>
#include "./expr_tokenizer.hpp"

namespace pepp::debug {

class Term {
public:
  enum class Type {
    Variable,
    Constant,
    Register,
    BinaryInfixOperator,
    UnaryPrefixOperator,
    FunctionCall,
    ParenExpr,
  };
  virtual ~Term() = 0;
  virtual uint16_t depth() const = 0;
  virtual Type type() const = 0;
  virtual std::strong_ordering operator<=>(const Term &rhs) const = 0;
  virtual QString to_string() const = 0;
  // std::set<std::weak_ptr<Term>> dependents;
};

struct Variable : public Term {
  ~Variable() = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const Variable &rhs) const;
  QString to_string() const override;
};

struct Register : public Term {};

struct Constant : public Term {
  Constant(const detail::UnsignedConstant &constant);
  ~Constant() = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  inline std::strong_ordering operator<=>(const Constant &rhs) const { return std::strong_ordering::equal; }
  detail::UnsignedConstant _val;
  QString to_string() const override;
};

struct BinaryInfix : public Term {
  ~BinaryInfix() = default;
  enum class Operators {
    DOT,         // a.b
    STAR_DOT,    // a->b, equivalent to (*a).b
    MULTIPY,     // a *b
    DIVIDE,      // a / b
    MODULO,      // a % b
    ADD,         // a + b
    SUBTRACT,    // a - b
    SHIFT_LEFT,  // a << b
    SHIFT_RIGHT, // a >> b
    LESS,
    LESS_OR_EQUAL,
    EQUAL,
    NOT_EQUAL,
    GREATER,
    GREATE_OR_EQUAL,
    BIT_AND,
    BIT_OR,
    BIT_XOR
  } op;
  std::shared_ptr<Term> arg1, arg2;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const BinaryInfix &rhs) const;
  QString to_string() const override;
};

struct UnaryPrefix : public Term {
  ~UnaryPrefix() = default;
  enum class Operator { PLUS, MINUS, DEREFERENCE, ADDRESS_OF } op;
  std::shared_ptr<Term> _arg1;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const UnaryPrefix &rhs) const;
  QString to_string() const override;
};

struct Expression : public Term {};
struct Parameters : public Term {};

struct Parenthesized : public Term {
  Parenthesized(std::shared_ptr<Term> term);
  ~Parenthesized() = default;
  uint16_t depth() const override;
  Type type() const override;

  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const Parenthesized &rhs) const;
  QString to_string() const override;
  std::shared_ptr<Term> _term;
};

} // namespace pepp::debug
