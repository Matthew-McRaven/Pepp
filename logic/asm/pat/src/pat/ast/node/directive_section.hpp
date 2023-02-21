#pragma once
#include "./directive.hpp"

namespace pat::ast::node {
class Section : public Directive {
public:
  struct Config {
    QString name = u"SECTION"_qs;
    bool emitAttributes = true;
  };

  struct Attributes {
    // Read, write, execute
    bool R = true, W = true, X = true;
  };

  explicit Section();
  Section(QSharedPointer<argument::Identifier> argument,
          FileLocation sourceLocation, QWeakPointer<Base> parent = {});
  Section(const Section &other);
  Section(Section &&other) noexcept;
  Section &operator=(Section other);
  friend void swap(Section &first, Section &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._config, second._config);
    swap(first._argument, second._argument);
    swap(first._attributes, second._attributes);
  }

  // ast::Value interface
  QSharedPointer<Value> clone() const override;
  bits::BitOrder endian() const override;
  quint64 size() const override;
  bool bits(QByteArray &out, bits::BitSelection src,
            bits::BitSelection dest) const override;
  bool bytes(QByteArray &out, qsizetype start, qsizetype length) const override;
  QString string() const override;

  // ast::node::Base interface
  const AddressSpan &addressSpan() const override;
  void updateAddressSpan(void *update) const override;
  bool emitsBytes() const override;
  void setEmitsBytes(bool emitBytes) override;

private:
  Config _config = {};
  QSharedPointer<argument::Identifier> _argument = nullptr;
  Attributes _attributes = {};
};
} // namespace pat::ast::node
