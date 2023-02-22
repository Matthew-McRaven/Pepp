#pragma once
#include "./instruction.hpp"

namespace pat::pep::ast::node {
template <typename ISA> class Instruction_R : public Instruction<ISA> {
public:
  explicit Instruction_R();
  Instruction_R(typename ISA::Mnemonic mnemonic,
                ::pat::ast::node::FileLocation sourceLocation,
                QWeakPointer<::pat::ast::node::Base> parent);
  Instruction_R(const Instruction_R &other);
  Instruction_R(Instruction_R &&other) noexcept;
  Instruction_R<ISA> &operator=(Instruction_R<ISA> other);
  friend void swap(Instruction_R<ISA> &first, Instruction_R<ISA> &second) {
    using std::swap;
    swap((Instruction<ISA> &)first, (Instruction<ISA> &)second);
  }

  // Value interface
  QSharedPointer<pat::ast::node::Base> clone() const override;
  quint64 size() const override;
  bool
  value(quint8 *dest, qsizetype length,
        bits::BitOrder destEndian = bits::BitOrder::BigEndian) const override;
  QString string() const override;
};

template <typename ISA> Instruction_R<ISA>::Instruction_R() {}

template <typename ISA>
Instruction_R<ISA>::Instruction_R(typename ISA::Mnemonic mnemonic,
                                  ::pat::ast::node::FileLocation sourceLocation,
                                  QWeakPointer<::pat::ast::node::Base> parent)
    : Instruction<ISA>(mnemonic, sourceLocation, parent) {}

template <typename ISA>
Instruction_R<ISA>::Instruction_R(const Instruction_R<ISA> &other)
    : Instruction<ISA>(other) {}

template <typename ISA>
Instruction_R<ISA>::Instruction_R(Instruction_R<ISA> &&other) noexcept {
  swap(*this, other);
}

template <typename ISA>
Instruction_R<ISA> &Instruction_R<ISA>::operator=(Instruction_R<ISA> other) {
  swap(*this, other);
  return *this;
}

template <typename ISA>
QSharedPointer<pat::ast::node::Base> Instruction_R<ISA>::clone() const {
  return QSharedPointer<Instruction_R<ISA>>::create(*this);
}

template <typename ISA> quint64 Instruction_R<ISA>::size() const { return 3; }

template <typename ISA>
bool Instruction_R<ISA>::value(quint8 *dest, qsizetype length,
                               bits::BitOrder destEndian) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA> QString Instruction_R<ISA>::string() const {
  throw std::logic_error("Unimplemented");
}

} // namespace pat::pep::ast::node
