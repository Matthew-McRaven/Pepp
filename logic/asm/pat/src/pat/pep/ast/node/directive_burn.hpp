#pragma once
#include "../../../ast/node/directive.hpp"
namespace pat::ast::argument {
class Hexadecimal;
}

namespace pat::pep::ast::node {
class Burn : public pat::ast::node::Directive {
public:
  explicit Burn();
  Burn(QSharedPointer<::pat::ast::argument::Hexadecimal> argument,
       ::pat::ast::node::FileLocation sourceLocation,
       QWeakPointer<::pat::ast::node::Base> parent);
  Burn(const Burn &other);
  Burn(Burn &&other) noexcept;
  Burn &operator=(Burn other);
  friend void swap(Burn &first, Burn &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._argument, second._argument);
  }

  struct ValidateResult {
    bool valid = true;
    QString errorMessage = u""_qs;
  };
  static ValidateResult validate_argument(
      QSharedPointer<const ::pat::ast::argument::Hexadecimal> argument);

  // Value interface
  QSharedPointer<pat::ast::node::Base> clone() const override;
  quint64 size() const override;
  bool
  value(quint8 *dest, qsizetype length,
        bits::BitOrder destEndian = bits::BitOrder::BigEndian) const override;
  QString string() const override;

  // Base interface
  const ::pat::ast::node::AddressSpan &addressSpan() const override;
  void updateAddressSpan(void *update) const override;
  bool emitsBytes() const override;
  void setEmitsBytes(bool emitBytes) override;

private:
  QSharedPointer<pat::ast::argument::Hexadecimal> _argument;
};
}; // namespace pat::pep::ast::node
