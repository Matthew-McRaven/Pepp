#pragma once
#include "./instruction.hpp"
#include "pat/ast/argument/base.hpp"

namespace pat::pep::ast::node {
template <typename ISA> class Instruction_RAAA : public Instruction<ISA> {
public:
  explicit Instruction_RAAA();
  Instruction_RAAA(typename ISA::Mnemonic mnemonic,
                   typename ISA::AddressingMode addressingMode,
                   ::pat::ast::node::FileLocation sourceLocation,
                   QWeakPointer<::pat::ast::node::Base> parent);
  Instruction_RAAA(const Instruction_RAAA &other);
  Instruction_RAAA(Instruction_RAAA &&other) noexcept;
  Instruction_RAAA<ISA> &operator=(Instruction_RAAA<ISA> other);
  friend void swap(Instruction_RAAA<ISA> &first,
                   Instruction_RAAA<ISA> &second) {
    using std::swap;
    swap((Instruction<ISA> &)first, (Instruction<ISA> &)second);
    swap(first._argument, second._argument);
    swap(first._addressingMode, second._addressingMode);
  }

  struct ValidateResult {
    bool valid = true;
    QString errorMessage = {};
  };
  static ValidateResult
  validateArgument(QSharedPointer<const pat::ast::argument::Base> argument);
  QSharedPointer<pat::ast::argument::Base> argument() const;
  void setArgument(QSharedPointer<pat::ast::argument::Base> argument);

  // Value interface
  QSharedPointer<pat::ast::node::Base> clone() const override;
  quint64 size() const override;
  bool
  value(quint8 *dest, qsizetype length,
        bits::BitOrder destEndian = bits::BitOrder::BigEndian) const override;
  QString string() const override;

private:
  QSharedPointer<pat::ast::argument::Base> _argument = {};
  typename ISA::AddressingMode _addressingMode = ISA::defaultAddressingMode();
};

template <typename ISA> Instruction_RAAA<ISA>::Instruction_RAAA() {}

template <typename ISA>
Instruction_RAAA<ISA>::Instruction_RAAA(
    typename ISA::Mnemonic mnemonic,
    typename ISA::AddressingMode addressingMode,
    ::pat::ast::node::FileLocation sourceLocation,
    QWeakPointer<::pat::ast::node::Base> parent)
    : Instruction<ISA>(mnemonic, sourceLocation, parent),
      _addressingMode(addressingMode) {}

template <typename ISA>
Instruction_RAAA<ISA>::Instruction_RAAA(const Instruction_RAAA<ISA> &other)
    : Instruction<ISA>(other), _argument(other._argument),
      _addressingMode(other._addressingMode) {}

template <typename ISA>
Instruction_RAAA<ISA>::Instruction_RAAA(
    Instruction_RAAA<ISA> &&other) noexcept {
  swap(*this, other);
}

template <typename ISA>
Instruction_RAAA<ISA> &
Instruction_RAAA<ISA>::operator=(Instruction_RAAA<ISA> other) {
  swap(*this, other);
  return *this;
}

template <typename ISA>
typename Instruction_RAAA<ISA>::ValidateResult
Instruction_RAAA<ISA>::validateArgument(
    QSharedPointer<const ::pat::ast::argument::Base> argument) {
  if (!argument->isNumeric())
    return {.valid = false, .errorMessage = u"Argument must be numeric"_qs};
  else if (!argument->isFixedSize())
    return {.valid = false, .errorMessage = u"Argument must be fixed size"_qs};
  else
    return {.valid = true};
}

template <typename ISA>
QSharedPointer<pat::ast::argument::Base>
Instruction_RAAA<ISA>::argument() const {
  return _argument;
}

template <typename ISA>
void Instruction_RAAA<ISA>::setArgument(
    QSharedPointer<::pat::ast::argument::Base> argument) {
  _argument = argument;
}

template <typename ISA>
QSharedPointer<pat::ast::node::Base> Instruction_RAAA<ISA>::clone() const {
  return QSharedPointer<Instruction_RAAA<ISA>>::create(*this);
}

template <typename ISA> quint64 Instruction_RAAA<ISA>::size() const {
  return 3;
}

template <typename ISA>
bool Instruction_RAAA<ISA>::value(quint8 *dest, qsizetype length,
                                  bits::BitOrder destEndian) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA> QString Instruction_RAAA<ISA>::string() const {
  throw std::logic_error("Unimplemented");
}

} // namespace pat::pep::ast::node
