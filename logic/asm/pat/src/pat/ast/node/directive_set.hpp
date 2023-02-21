#pragma once
#include "./directive.hpp"

namespace pat::ast::node {
class Set : public Directive {
public:
  enum class SymbolStyle {
    kAsArgument, // .EQUATE symbol, 25
    kAsPrefix    // symbol: .EQUATE 25
  };
  struct Config {
    QString name = u"EQUATE"_qs;
    SymbolStyle style = SymbolStyle::kAsPrefix;
  };

  explicit Set();
  Set(QSharedPointer<argument::Base> argument, FileLocation sourceLocation,
      QWeakPointer<Base> parent = {});
  Set(const Set &other);
  Set(Set &&other) noexcept;
  Set &operator=(Set other);
  friend void swap(Set &first, Set &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._config, second._config);
    swap(first._argument, second._argument);
  }

  const Config &config() const;
  void setConfig(Config config);

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
  QSharedPointer<argument::Base> _argument; // numeric && fixedSize
};
} // namespace pat::ast::node
