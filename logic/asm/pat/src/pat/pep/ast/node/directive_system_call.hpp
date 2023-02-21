#pragma once
#include "./directive_symbolic.hpp"
namespace pat::ast::argument {
class Symbolic;
}

namespace pat::pep::ast::node {
class SCall : public pat::pep::ast::node::Symbolic {
public:
  explicit SCall();
  SCall(QSharedPointer<::pat::ast::argument::Symbolic> argument,
        ::pat::ast::node::FileLocation sourceLocation,
        QWeakPointer<::pat::ast::node::Base> parent);
  SCall(const SCall &other);
  SCall(SCall &&other) noexcept;
  SCall &operator=(SCall other);
  friend void swap(SCall &first, SCall &second) {
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

class USCall : public pat::pep::ast::node::Symbolic {
public:
  explicit USCall();
  USCall(QSharedPointer<::pat::ast::argument::Symbolic> argument,
         ::pat::ast::node::FileLocation sourceLocation,
         QWeakPointer<::pat::ast::node::Base> parent);
  USCall(const USCall &other);
  USCall(USCall &&other) noexcept;
  USCall &operator=(USCall other);
  friend void swap(USCall &first, USCall &second) {
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
