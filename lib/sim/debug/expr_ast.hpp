#pragma once
#include <QtCore>
#include <memory>
#include <set>
#include "./expr_tokenizer.hpp"
#include "expr_eval.hpp"

namespace pepp::debug {

// When creating a shared_ptr<Term> (or derived), must immediately call link() to link _dependents.
class Term : public std::enable_shared_from_this<Term> {
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

  // Add this to nested/member term's _dependents, forming a bidirectional tree.
  // Must be called after creating a new shared_ptr!
  virtual void link() = 0;
  // Insert a pointer into _dependents.
  void add_dependent(std::weak_ptr<Term> term);
  // Returns true if t is in this _depedents set (a direct dependency).
  // Does not account for transitive dependencies!!
  bool dependency_of(std::shared_ptr<Term> term);

  // Evaluate this AST to a value, marking elements as not dirty as they are re-evaluated.
  virtual TypedBits evaluate(EvaluationMode mode) = 0;

  // Recurses upwards, marking parents as dirty as well as self.
  void mark_tree_dirty();

  // Mark self as dirty.
  virtual void mark_dirty() = 0;
  virtual bool dirty() const = 0;

protected:
  // Track which terms may be made dirty if the current term's value changes.
  // Use weak pointers to prevent extending lifetimes of dependents.
  // Some dependents may be discarded during parsing,
  std::vector<std::weak_ptr<Term>> _dependents;
};

struct Variable : public Term {
  Variable(const detail::Identifier &ident);
  Variable(QString name);
  ~Variable() = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const Variable &rhs) const;
  QString to_string() const override;
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  void mark_dirty() override;
  bool dirty() const override;

  QString _name;
};

struct Register : public Term {};

struct Constant : public Term {
  Constant(const TypedBits &bits, detail::UnsignedConstant::Format format_hint = detail::UnsignedConstant::Format::Dec);
  ~Constant() = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const Constant &rhs) const;
  QString to_string() const override;
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  void mark_dirty() override;
  bool dirty() const override;

  detail::UnsignedConstant::Format _format_hint;
  TypedBits _value;
};

struct BinaryInfix : public Term {
  enum class Operators {
    DOT,         // a.b
    STAR_DOT,    // a->b, equivalent to (*a).b
    MULTIPLY,    // a *b
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
    GREATER_OR_EQUAL,
    BIT_AND,
    BIT_OR,
    BIT_XOR
  };
  BinaryInfix(Operators op, std::shared_ptr<Term> arg1, std::shared_ptr<Term> arg2);
  ~BinaryInfix() = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const BinaryInfix &rhs) const;
  QString to_string() const override;
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  void mark_dirty() override;
  bool dirty() const override;

  Operators _op;
  std::shared_ptr<Term> _arg1, _arg2;
  EvaluationCache _state;
};
std::optional<BinaryInfix::Operators> string_to_binary_infix(QStringView);

struct UnaryPrefix : public Term {
  enum class Operators { PLUS, MINUS, DEREFERENCE, ADDRESS_OF, NOT, NEGATE };
  UnaryPrefix(Operators op, std::shared_ptr<Term> arg);
  ~UnaryPrefix() = default;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const UnaryPrefix &rhs) const;
  uint16_t depth() const override;
  Type type() const override;
  QString to_string() const override;
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  void mark_dirty() override;
  bool dirty() const override;

  Operators _op;
  std::shared_ptr<Term> _arg;
  EvaluationCache _state;
};
std::optional<UnaryPrefix::Operators> string_to_unary_prefix(QStringView);

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
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  void mark_dirty() override;
  bool dirty() const override;

  std::shared_ptr<Term> _term;
};

} // namespace pepp::debug
