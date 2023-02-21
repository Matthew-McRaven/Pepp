#pragma once
#include "pat/ast/node/base.hpp"
#include "symbol/entry.hpp"
namespace pat::pep::ast::node {
template <typename ISA> class Instruction : public pat::ast::node::Base {
public:
  explicit Instruction();
  Instruction(typename ISA::Mnemonic mnemonic,
              ::pat::ast::node::FileLocation sourceLocation,
              QWeakPointer<::pat::ast::node::Base> parent);
  friend void swap(Instruction &first, Instruction &second) {
    using std::swap;
    swap((Base &)first, (Base &)second);
    swap(first._emitsBytes, second._emitsBytes);
    swap(first._mnemonic, second._mnemonic);
    swap(first._symbol, second._symbol);
    swap(first._comment, second._comment);
  }

  std::optional<QString> comment() const;
  void setComment(std::optional<QString>);
  QSharedPointer<const symbol::Entry> symbol() const;
  QSharedPointer<symbol::Entry> symbol();
  void setSymbol(QSharedPointer<symbol::Entry> symbol);

  // Value interface
  virtual QSharedPointer<pat::ast::Value> clone() const override = 0;
  bits::BitOrder endian() const override;
  virtual quint64 size() const override = 0;
  virtual bool bits(QByteArray &out, bits::BitSelection src,
                    bits::BitSelection dest) const override = 0;
  virtual bool bytes(QByteArray &out, qsizetype start,
                     qsizetype length) const override = 0;
  virtual QString string() const override = 0;

  // Base interface
  const pat::ast::node::AddressSpan &addressSpan() const override;
  void updateAddressSpan(void *update) const override;
  bool emitsBytes() const override;
  void setEmitsBytes(bool emitBytes) override;

protected:
  Instruction(const Instruction<ISA> &other);
  Instruction<ISA> &operator=(Instruction<ISA> &other);

  bool _emitsBytes = true;
  typename ISA::Mnemonic _mnemonic = ISA::defaultMnemonic();
  QSharedPointer<symbol::Entry> _symbol = nullptr;
  std::optional<QString> _comment = std::nullopt;
};

template <typename ISA> Instruction<ISA>::Instruction() : Base() {}

template <typename ISA>
Instruction<ISA>::Instruction(typename ISA::Mnemonic mnemonic,
                              pat::ast::node::FileLocation sourceLocation,
                              QWeakPointer<Base> parent)
    : Base(sourceLocation, parent), _mnemonic(mnemonic) {}

template <typename ISA>
std::optional<QString> Instruction<ISA>::comment() const {
  return _comment;
}

template <typename ISA>
void Instruction<ISA>::setComment(std::optional<QString> comment) {
  _comment = comment;
}

template <typename ISA>
QSharedPointer<const symbol::Entry> Instruction<ISA>::symbol() const {
  return _symbol;
}

template <typename ISA>
QSharedPointer<symbol::Entry> Instruction<ISA>::symbol() {
  return _symbol;
}

template <typename ISA>
void Instruction<ISA>::setSymbol(QSharedPointer<symbol::Entry> symbol) {
  _symbol = symbol;
}

template <typename ISA> bits::BitOrder Instruction<ISA>::endian() const {
  return bits::BitOrder::BigEndian;
}

template <typename ISA>
const pat::ast::node::AddressSpan &Instruction<ISA>::addressSpan() const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA>
void Instruction<ISA>::updateAddressSpan(void *update) const {
  throw std::logic_error("Unimplemented");
}

template <typename ISA> bool Instruction<ISA>::emitsBytes() const {
  return _emitsBytes;
}

template <typename ISA> void Instruction<ISA>::setEmitsBytes(bool emitBytes) {
  _emitsBytes = emitBytes;
}

template <typename ISA>
Instruction<ISA>::Instruction(const Instruction<ISA> &other)
    : Base(other), _emitsBytes(other._emitsBytes), _mnemonic(other._mnemonic),
      _symbol(other._symbol), _comment(other._comment) {}

template <typename ISA>
Instruction<ISA> &Instruction<ISA>::operator=(Instruction<ISA> &other) {
  Base::operator=(other);
  this->_emitsBytes = other._emitsBytes;
  this->_mnemonic = other._mnemonic;
  this->_symbol = other._symbol;
  this->_comment = other._comment;
}

} // namespace pat::pep::ast::node
