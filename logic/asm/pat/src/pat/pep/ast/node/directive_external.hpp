#pragma once
#include "../../../ast/node/directive.hpp"
#include "pat/pep/ast/node/directive_symbolic.hpp"
namespace pat::ast::argument {
class Symbolic;
}

namespace pat::pep::ast::node {
class Export : public pat::pep::ast::node::Symbolic {
public:
  explicit Export();
  Export(QSharedPointer<::pat::ast::argument::Symbolic> argument,
         ::pat::ast::node::FileLocation sourceLocation,
         QWeakPointer<::pat::ast::node::Base> parent);
  Export(const Export &other);
  Export(Export &&other) noexcept;
  Export &operator=(Export other);
  friend void swap(Export &first, Export &second) {
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
  QSharedPointer<pat::ast::node::Base> clone() const override;
  QString string() const override;
};

class Import : public ::pat::pep::ast::node::Symbolic {
public:
  explicit Import();
  Import(QSharedPointer<::pat::ast::argument::Symbolic> argument,
         ::pat::ast::node::FileLocation sourceLocation,
         QWeakPointer<::pat::ast::node::Base> parent);
  Import(const Import &other);
  Import(Import &&other) noexcept;
  Import &operator=(Import other);
  friend void swap(Import &first, Import &second) {
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
  QSharedPointer<pat::ast::node::Base> clone() const override;
  QString string() const override;
};
} // namespace pat::pep::ast::node
