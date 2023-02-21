#pragma once
#include "./base.hpp"
#include <QtCore>
namespace symbol {
class Entry;
}

namespace pat::ast {

namespace argument {
class Base;
class Identifier;
} // namespace argument

namespace node {

class Directive : public node::Base {
public:
  explicit Directive();
  Directive(FileLocation sourceLocation, QWeakPointer<Base> parent = {});
  friend void swap(Directive &first, Directive &second) {
    using std::swap;
    swap((Base &)first, (Base &)second);
    swap(first._symbol, second._symbol);
    swap(first._comment, second._comment);
  }

  // ast::Value interface
  virtual QSharedPointer<pat::ast::Value> clone() const override = 0;
  virtual bits::BitOrder endian() const override = 0;
  virtual quint64 size() const override = 0;
  virtual bool bits(QByteArray &out, bits::BitSelection src,
                    bits::BitSelection dest) const override = 0;
  virtual bool bytes(QByteArray &out, qsizetype start,
                     qsizetype length) const override = 0;
  virtual QString string() const override = 0;

  // ast::node::Base interface
  virtual const AddressSpan &addressSpan() const override = 0;
  virtual void updateAddressSpan(void *update) const override = 0;
  virtual bool emitsBytes() const override = 0;
  virtual void setEmitsBytes(bool emitBytes) override = 0;

  // Helpers implemented locally
  QSharedPointer<const symbol::Entry> symbol() const;
  QSharedPointer<symbol::Entry> symbol();
  void setSymbol(QSharedPointer<symbol::Entry> symbol);
  std::optional<QString> comment() const;
  void setComment(std::optional<QString> comment);

protected:
  Directive(const Directive &other);
  Directive &operator=(const Directive &other);
  QSharedPointer<symbol::Entry> _symbol = {};
  std::optional<QString> _comment = std::nullopt;
};

} // namespace node
} // namespace pat::ast
