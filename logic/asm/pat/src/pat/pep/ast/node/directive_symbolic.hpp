#pragma once
#include "../../../ast/node/directive.hpp"
namespace pat::ast::argument {
class Symbolic;
}

namespace pat::pep::ast::node {
class Symbolic : public pat::ast::node::Directive {
public:
  explicit Symbolic();
  Symbolic(QSharedPointer<::pat::ast::argument::Symbolic> argument,
           ::pat::ast::node::FileLocation sourceLocation,
           QWeakPointer<::pat::ast::node::Base> parent);

  friend void swap(Symbolic &first, Symbolic &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._argument, second._argument);
  }

  // Value interface
  virtual QSharedPointer<pat::ast::node::Base> clone() const override = 0;
  quint64 size() const override;
  bool
  value(quint8 *dest, qsizetype length,
        bits::BitOrder destEndian = bits::BitOrder::BigEndian) const override;
  virtual QString string() const override = 0;

  // Base interface
  const ::pat::ast::node::AddressSpan &addressSpan() const override;
  void updateAddressSpan(void *update) const override;
  bool emitsBytes() const override;
  void setEmitsBytes(bool emitBytes) override;

protected:
  Symbolic(const Symbolic &other);
  // Symbolic(Symbolic &&other) noexcept;
  Symbolic &operator=(Symbolic &other);
  QSharedPointer<pat::ast::argument::Symbolic> _argument;
};

} // namespace pat::pep::ast::node
