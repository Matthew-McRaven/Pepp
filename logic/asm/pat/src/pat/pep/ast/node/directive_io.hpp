#pragma once
#include "./directive_symbolic.hpp"
namespace pat::ast::argument {
class Symbolic;
}

namespace pat::pep::ast::node {
class Input : public pat::pep::ast::node::Symbolic {
public:
  explicit Input();
  Input(QSharedPointer<::pat::ast::argument::Symbolic> argument,
        ::pat::ast::node::FileLocation sourceLocation,
        QWeakPointer<::pat::ast::node::Base> parent);
  Input(const Input &other);
  Input(Input &&other) noexcept;
  Input &operator=(Input other);
  friend void swap(Input &first, Input &second) {
    using std::swap;
    swap((node::Symbolic &)first, (node::Symbolic &)second);
  }

  struct ValidateResult {
    bool valid = true;
    QString errorMessage = {};
  };
  static ValidateResult validate_argument(
      QSharedPointer<const ::pat::ast::argument::Symbolic> argument);

  // Value interface
  QSharedPointer<pat::ast::Value> clone() const;
  QString string() const;

private:
  QSharedPointer<pat::ast::argument::Symbolic> _argument;
};

class Output : public pat::pep::ast::node::Symbolic {
public:
  explicit Output();
  Output(QSharedPointer<::pat::ast::argument::Symbolic> argument,
         ::pat::ast::node::FileLocation sourceLocation,
         QWeakPointer<::pat::ast::node::Base> parent);
  Output(const Output &other);
  Output(Output &&other) noexcept;
  Output &operator=(Output other);
  friend void swap(Output &first, Output &second) {
    using std::swap;
    swap((node::Symbolic &)first, (node::Symbolic &)second);
  }

  struct ValidateResult {
    bool valid = true;
    QString errorMessage = {};
  };
  static ValidateResult validate_argument(
      QSharedPointer<const ::pat::ast::argument::Symbolic> argument);

  // Value interface
  QSharedPointer<pat::ast::Value> clone() const;
  QString string() const;
};
} // namespace pat::pep::ast::node
