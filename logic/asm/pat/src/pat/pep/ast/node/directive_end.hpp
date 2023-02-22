#pragma once
#include "../../../ast/node/directive.hpp"
namespace pat::pep::ast::node {
class End : public pat::ast::node::Directive {
public:
  explicit End();
  End(bool doNotEmit, ::pat::ast::node::FileLocation sourceLocation,
      QWeakPointer<::pat::ast::node::Base> parent);
  End(const End &other);
  End(End &&other) noexcept;
  End &operator=(End other);
  friend void swap(End &first, End &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._doNotEmit, second._doNotEmit);
  }

  bool doNotEmit() const;
  void setDoNotEmit(bool doNotEmit);
  // Value interface
  QSharedPointer<Base> clone() const override;
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
  bool _doNotEmit = false;
};
}; // namespace pat::pep::ast::node
