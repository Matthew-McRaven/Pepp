#pragma once
#include "./directive.hpp"

namespace pat::ast::node {
struct Align : public Directive {
  struct Config {
    QString name = u"ALIGN"_qs;
    bool allowDefaultPad = true;
    quint8 defaultPad = 0;
  };

  explicit Align();
  Align(QSharedPointer<argument::Base> argument, FileLocation sourceLocation,
        QWeakPointer<node::Base> parent);
  Align(const Align &other);
  Align(Align &&other) noexcept;
  Align &operator=(Align other);
  friend void swap(Align &first, Align &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._config, second._config);
    swap(first._argument, second._argument);
    swap(first._pad, second._pad);
    swap(first._emitsBytes, second._emitsBytes);
  }

  const Config &config() const;
  void setConfig(Config config);
  void setPad(QSharedPointer<argument::Base> pad);

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
  QSharedPointer<argument::Base> _argument =
      {}; // numeric && fixedSize. Must be initialized.
  QSharedPointer<argument::Base> _pad =
      {}; // numeric && fixedSize. Uses default config if nullptr &&
          // config.allowDefaultPad
  bool _emitsBytes = true;
};
} // namespace pat::ast::node
