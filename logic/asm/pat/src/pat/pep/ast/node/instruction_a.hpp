#pragma once
#include "./instruction.hpp"
#include "pat/ast/argument/base.hpp"

namespace pat::pep::ast::node {
template <typename ISA> class Instruction_A : public Instruction<ISA> {
public:
  explicit Instruction_A();
  Instruction_A(typename ISA::Mnemonic mnemonic,
                typename ISA::AddressingMode addressingMode,
                ::pat::ast::node::FileLocation sourceLocation,
                QWeakPointer<::pat::ast::node::Base> parent);
  Instruction_A(const Instruction_A &other);
  Instruction_A(Instruction_A &&other) noexcept;
  Instruction_A<ISA> &operator=(Instruction_A<ISA> other);
  friend void swap(Instruction_A<ISA> &first, Instruction_A<ISA> &second) {
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
  QSharedPointer<pat::ast::Value> clone() const override;
  bits::BitOrder endian() const override;
  quint64 size() const override;
  bool bits(QByteArray &out, bits::BitSelection src,
            bits::BitSelection dest) const override;
  bool bytes(QByteArray &out, qsizetype start, qsizetype length) const override;
  QString string() const override;

private:
  QSharedPointer<pat::ast::argument::Base> _argument = {};
  typename ISA::AddressingMode _addressingMode = ISA::defaultAddressingMode();
};

template <typename ISA> Instruction_A<ISA>::Instruction_A() {}

template <typename ISA>
Instruction_A<ISA>::Instruction_A(typename ISA::Mnemonic mnemonic,
                                  typename ISA::AddressingMode addressingMode,
                                  ::pat::ast::node::FileLocation sourceLocation,
                                  QWeakPointer<::pat::ast::node::Base> parent)
    : Instruction<ISA>(mnemonic, sourceLocation, parent),
      _addressingMode(addressingMode) {}

template <typename ISA>
Instruction_A<ISA>::Instruction_A(const Instruction_A<ISA> &other)
    : Instruction<ISA>(other), _addressingMode(other._addressingMode),
      _argument(other._argument) {}

template <typename ISA>
Instruction_A<ISA>::Instruction_A(Instruction_A<ISA> &&other) noexcept {
  swap(*this, other);
}

template <typename ISA>
Instruction_A<ISA> &Instruction_A<ISA>::operator=(Instruction_A<ISA> other) {
  swap(*this, other);
  return *this;
}

template <typename ISA>
typename Instruction_A<ISA>::ValidateResult
Instruction_A<ISA>::validateArgument(
    QSharedPointer<const ::pat::ast::argument::Base> argument) {
  if (!argument->isNumeric())
    return {.valid = false, .errorMessage = u"Argument must be numeric"_qs};
  else if (!argument->isFixedSize())
    return {.valid = false, .errorMessage = u"Argument must be fixed size"_qs};
  else
    return {.valid = true};
}

template <typename ISA>
QSharedPointer<pat::ast::argument::Base> Instruction_A<ISA>::argument() const {
  return _argument;
}

template <typename ISA>
void Instruction_A<ISA>::setArgument(
    QSharedPointer<::pat::ast::argument::Base> argument) {
  _argument = argument;
}

template <typename ISA>
QSharedPointer<pat::ast::Value> Instruction_A<ISA>::clone() const {
  return QSharedPointer<Instruction_A<ISA>>::create(*this);
}

template <typename ISA> bits::BitOrder Instruction_A<ISA>::endian() const {
  return bits::BitOrder::BigEndian;
}

template <typename ISA> quint64 Instruction_A<ISA>::size() const { return 3; }

template <typename ISA>
bool Instruction_A<ISA>::bits(QByteArray &out, bits::BitSelection src,
                              bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA>
bool Instruction_A<ISA>::bytes(QByteArray &out, qsizetype start,
                               qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA> QString Instruction_A<ISA>::string() const {
  throw std::logic_error("Unimplemented");
}

} // namespace pat::pep::ast::node
