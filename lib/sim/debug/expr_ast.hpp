#pragma once
#include <QtCore>
#include <memory>
#include <set>
#include "./expr_tokenizer.hpp"
#include "expr_eval.hpp"

namespace pepp::debug {

struct ConstantTermVisitor;
struct MutatingTermVisitor;
// When creating a shared_ptr<Term> (or derived), must immediately call link() to update _dependents.
// Prefer creation through ExpressionCache, which handles this on your behalf.
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
  bool dependency_of(std::shared_ptr<Term> term) const;

  // Evaluate this AST to a value, marking elements as not dirty as they are re-evaluated.
  virtual TypedBits evaluate(EvaluationMode mode) = 0;
  // Do not recurse, only report local const/volatile qualifiers.
  virtual int cv_qualifiers() const = 0;

  // Recurses upwards, marking parents as dirty as well as self.
  void mark_tree_dirty();

  // Mark self as dirty.
  virtual void mark_dirty() = 0;
  virtual bool dirty() const = 0;

  virtual void accept(MutatingTermVisitor &visitor) = 0;
  virtual void accept(ConstantTermVisitor &visitor) const = 0;

protected:
  // Track which terms may be made dirty if the current term's value changes.
  // Use weak pointers to prevent extending lifetimes of dependents.
  // Some dependents may be discarded during parsing,
  std::vector<std::weak_ptr<Term>> _dependents;
};

struct Variable : public Term {
  explicit Variable(const detail::Identifier &ident);
  explicit Variable(QString name);
  ~Variable() override = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const Variable &rhs) const;
  QString to_string() const override;

  void link() override;

  TypedBits evaluate(EvaluationMode mode) override;
  int cv_qualifiers() const override;

  void mark_dirty() override;
  bool dirty() const override;
  void accept(MutatingTermVisitor &visitor) override;
  void accept(ConstantTermVisitor &visitor) const override;
  const QString name;
};

struct Register : public Term {};

struct Constant : public Term {
  template <std::integral I>
  explicit Constant(I bits, detail::UnsignedConstant::Format format_hint = detail::UnsignedConstant::Format::Dec)
      : format_hint(format_hint), value(from_int(bits)) {}
  explicit Constant(const TypedBits &bits,
                    detail::UnsignedConstant::Format format_hint = detail::UnsignedConstant::Format::Dec);
  ~Constant() override = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const Constant &rhs) const;
  QString to_string() const override;
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  int cv_qualifiers() const override;

  void mark_dirty() override;
  bool dirty() const override;
  void accept(MutatingTermVisitor &visitor) override;
  void accept(ConstantTermVisitor &visitor) const override;

  const detail::UnsignedConstant::Format format_hint;
  const TypedBits value;
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
  BinaryInfix(Operators op, std::shared_ptr<Term> lhs, std::shared_ptr<Term> rhs);
  ~BinaryInfix() override = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const BinaryInfix &rhs) const;
  QString to_string() const override;
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  int cv_qualifiers() const override;

  void mark_dirty() override;
  bool dirty() const override;

  void accept(MutatingTermVisitor &visitor) override;
  void accept(ConstantTermVisitor &visitor) const override;

  const Operators op;
  // Constant pointer to mutable object.
  const std::shared_ptr<Term> lhs, rhs;

private:
  EvaluationCache _state;
};
std::optional<BinaryInfix::Operators> string_to_binary_infix(QStringView);

struct UnaryPrefix : public Term {
  enum class Operators { PLUS, MINUS, DEREFERENCE, ADDRESS_OF, NOT, NEGATE };
  UnaryPrefix(Operators op, std::shared_ptr<Term> arg);
  ~UnaryPrefix() override = default;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const UnaryPrefix &rhs) const;
  uint16_t depth() const override;
  Type type() const override;
  QString to_string() const override;
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  int cv_qualifiers() const override;

  void mark_dirty() override;
  bool dirty() const override;

  void accept(MutatingTermVisitor &visitor) override;
  void accept(ConstantTermVisitor &visitor) const override;
  const Operators op;
  const std::shared_ptr<Term> arg;

private:
  EvaluationCache _state;
};
std::optional<UnaryPrefix::Operators> string_to_unary_prefix(QStringView);

struct Parenthesized : public Term {
  explicit Parenthesized(std::shared_ptr<Term> term);
  ~Parenthesized() override = default;
  uint16_t depth() const override;
  Type type() const override;
  std::strong_ordering operator<=>(const Term &rhs) const override;
  std::strong_ordering operator<=>(const Parenthesized &rhs) const;
  QString to_string() const override;
  void link() override;
  TypedBits evaluate(EvaluationMode mode) override;
  int cv_qualifiers() const override;

  void mark_dirty() override;
  bool dirty() const override;
  void accept(MutatingTermVisitor &visitor) override;
  void accept(ConstantTermVisitor &visitor) const override;

  const std::shared_ptr<Term> term;
};

// If you add a new AST node type, you'll need to add new handlers to these visitor classes.
struct MutatingTermVisitor {
  virtual void accept(Variable &node) = 0;
  virtual void accept(Constant &node) = 0;
  virtual void accept(BinaryInfix &node) = 0;
  virtual void accept(UnaryPrefix &node) = 0;
  virtual void accept(Parenthesized &node) = 0;
};
struct ConstantTermVisitor {
  virtual void accept(const Variable &node) = 0;
  virtual void accept(const Constant &node) = 0;
  virtual void accept(const BinaryInfix &node) = 0;
  virtual void accept(const UnaryPrefix &node) = 0;
  virtual void accept(const Parenthesized &node) = 0;
};

} // namespace pepp::debug
