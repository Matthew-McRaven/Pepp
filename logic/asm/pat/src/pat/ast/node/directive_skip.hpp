#pragma once
#include "./directive.hpp"

namespace pat::ast::node {
class Skip : public Directive {
public:
  struct Config {
    QString name = u"BLOCK"_qs;
    bool emitFill = false;
    bool useDefaultFill = true;
    quint8 defaultFill = 0;
  };
  explicit Skip();
  Skip(QSharedPointer<argument::Base> argument, FileLocation sourceLocation,
       QWeakPointer<Base> parent = {});
  Skip(const Skip &other);
  Skip(Skip &&other) noexcept;
  Skip &operator=(Skip other);
  friend void swap(Skip &first, Skip &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._config, second._config);
    swap(first._argument, second._argument);
    swap(first._fill, second._fill);
    swap(first._emitsBytes, second._emitsBytes);
  }

  const Config &config() const;
  void setConfig(Config config);
  QSharedPointer<const argument::Base> fill() const;
  void setFill(QSharedPointer<argument::Base> fill);

  struct ValidateResult {
    bool valid = true;
    QString errorMessage = {};
  };
  static ValidateResult
  validate_argument(QSharedPointer<const argument::Base> argument);

  // ast::Value interface
  QSharedPointer<Base> clone() const override;
  quint64 size() const override;
  bool
  value(quint8 *dest, qsizetype length,
        bits::BitOrder destEndian = bits::BitOrder::BigEndian) const override;
  QString string() const override;

  // ast::node::Base interface
  const AddressSpan &addressSpan() const override;
  void updateAddressSpan(void *update) const override;
  bool emitsBytes() const override;
  void setEmitsBytes(bool emitBytes) override;

private:
  Config _config = {};
  QSharedPointer<argument::Base> _argument = nullptr; // numeric && fixedSize
  QSharedPointer<argument::Base> _fill =
      nullptr; // numeric && fixedSize. Uses default config if nullptr &&
               // config.allowDefaultPad
  bool _emitsBytes = true;
};
} // namespace pat::ast::node
