#pragma once
#include "./instruction.hpp"
#include "pat/ast/argument/base.hpp"

namespace pat::pep::ast::node {
template <typename ISA> class Instruction_AAA : public Instruction<ISA> {
public:
  explicit Instruction_AAA();
  Instruction_AAA(typename ISA::Mnemonic mnemonic,
                  typename ISA::AddressingMode addressingMode,
                  ::pat::ast::node::FileLocation sourceLocation,
                  QWeakPointer<::pat::ast::node::Base> parent);
  Instruction_AAA(const Instruction_AAA &other);
  Instruction_AAA(Instruction_AAA &&other) noexcept;
  Instruction_AAA<ISA> &operator=(Instruction_AAA<ISA> other);
  friend void swap(Instruction_AAA<ISA> &first, Instruction_AAA<ISA> &second) {
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

template <typename ISA> Instruction_AAA<ISA>::Instruction_AAA() {}

template <typename ISA>
Instruction_AAA<ISA>::Instruction_AAA(
    typename ISA::Mnemonic mnemonic,
    typename ISA::AddressingMode addressingMode,
    ::pat::ast::node::FileLocation sourceLocation,
    QWeakPointer<::pat::ast::node::Base> parent)
    : Instruction<ISA>(mnemonic, sourceLocation, parent),
      _addressingMode(addressingMode) {}

template <typename ISA>
Instruction_AAA<ISA>::Instruction_AAA(const Instruction_AAA<ISA> &other)
    : Instruction<ISA>(other), _argument(other._argument),
      _addressingMode(other._addressingMode) {}

template <typename ISA>
Instruction_AAA<ISA>::Instruction_AAA(Instruction_AAA<ISA> &&other) noexcept {
  swap(*this, other);
}

template <typename ISA>
Instruction_AAA<ISA> &
Instruction_AAA<ISA>::operator=(Instruction_AAA<ISA> other) {
  swap(*this, other);
  return *this;
}

template <typename ISA>
typename Instruction_AAA<ISA>::ValidateResult
Instruction_AAA<ISA>::validateArgument(
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
Instruction_AAA<ISA>::argument() const {
  return _argument;
}

template <typename ISA>
void Instruction_AAA<ISA>::setArgument(
    QSharedPointer<::pat::ast::argument::Base> argument) {
  _argument = argument;
}

template <typename ISA>
QSharedPointer<pat::ast::Value> Instruction_AAA<ISA>::clone() const {
  return QSharedPointer<Instruction_AAA<ISA>>::create(*this);
}

template <typename ISA> bits::BitOrder Instruction_AAA<ISA>::endian() const {
  return bits::BitOrder::BigEndian;
}

template <typename ISA> quint64 Instruction_AAA<ISA>::size() const { return 3; }

template <typename ISA>
bool Instruction_AAA<ISA>::bits(QByteArray &out, bits::BitSelection src,
                                bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA>
bool Instruction_AAA<ISA>::bytes(QByteArray &out, qsizetype start,
                                 qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA> QString Instruction_AAA<ISA>::string() const {
  throw std::logic_error("Unimplemented");
}

} // namespace pat::pep::ast::node
