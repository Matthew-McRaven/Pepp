#pragma once
#include "./instruction.hpp"

namespace pat::pep::ast::node {
template <typename ISA> class Instruction_U : public Instruction<ISA> {
public:
  explicit Instruction_U();
  Instruction_U(typename ISA::Mnemonic mnemonic,
                ::pat::ast::node::FileLocation sourceLocation,
                QWeakPointer<::pat::ast::node::Base> parent);
  Instruction_U(const Instruction_U &other);
  Instruction_U(Instruction_U &&other) noexcept;
  Instruction_U<ISA> &operator=(Instruction_U<ISA> other);
  friend void swap(Instruction_U<ISA> &first, Instruction_U<ISA> &second) {
    using std::swap;
    swap((Instruction<ISA> &)first, (Instruction<ISA> &)second);
  }

  // Value interface
  QSharedPointer<pat::ast::Value> clone() const override;
  bits::BitOrder endian() const override;
  quint64 size() const override;
  bool bits(QByteArray &out, bits::BitSelection src,
            bits::BitSelection dest) const override;
  bool bytes(QByteArray &out, qsizetype start, qsizetype length) const override;
  QString string() const override;
};

template <typename ISA> Instruction_U<ISA>::Instruction_U() {}

template <typename ISA>
Instruction_U<ISA>::Instruction_U(typename ISA::Mnemonic mnemonic,
                                  ::pat::ast::node::FileLocation sourceLocation,
                                  QWeakPointer<::pat::ast::node::Base> parent)
    : Instruction<ISA>(mnemonic, sourceLocation, parent) {}

template <typename ISA>
Instruction_U<ISA>::Instruction_U(const Instruction_U<ISA> &other)
    : Instruction<ISA>(other) {}

template <typename ISA>
Instruction_U<ISA>::Instruction_U(Instruction_U<ISA> &&other) noexcept {
  swap(*this, other);
}

template <typename ISA>
Instruction_U<ISA> &Instruction_U<ISA>::operator=(Instruction_U<ISA> other) {
  swap(*this, other);
  return *this;
}

template <typename ISA>
QSharedPointer<pat::ast::Value> Instruction_U<ISA>::clone() const {
  return QSharedPointer<Instruction_U<ISA>>::create(*this);
}

template <typename ISA> bits::BitOrder Instruction_U<ISA>::endian() const {
  return bits::BitOrder::BigEndian;
}

template <typename ISA> quint64 Instruction_U<ISA>::size() const { return 3; }

template <typename ISA>
bool Instruction_U<ISA>::bits(QByteArray &out, bits::BitSelection src,
                              bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA>
bool Instruction_U<ISA>::bytes(QByteArray &out, qsizetype start,
                               qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA> QString Instruction_U<ISA>::string() const {
  throw std::logic_error("Unimplemented");
}

} // namespace pat::pep::ast::node
