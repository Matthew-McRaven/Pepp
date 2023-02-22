#pragma once
#include "./directive.hpp"

namespace pat::ast::node {
struct Byte1 : public Directive {
  struct Config {
    QString name = u"BYTE"_qs;
  };
  explicit Byte1();
  Byte1(QList<QSharedPointer<const argument::Base>> argument,
        FileLocation sourceLocation, QWeakPointer<Base> parent);
  Byte1(const Byte1 &other);
  Byte1(Byte1 &&other) noexcept;
  Byte1 &operator=(Byte1 other);
  friend void swap(Byte1 &first, Byte1 &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._config, second._config);
    swap(first._argument, second._argument);
    swap(first._emitsBytes, second._emitsBytes);
  }

  const Config &config() const;
  void setConfig(Config config);
  struct ValidateResult {
    bool valid = true;
    qsizetype argumentIndex = 0;
    QString errorMessage = {};
  };
  static ValidateResult
  validate_argument(const QList<QSharedPointer<const argument::Base>> argument);

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
  QList<QSharedPointer<const argument::Base>>
      _argument; // numeric && fixedSize, truncated to 1 byte.
  bool _emitsBytes = true;
};

struct Byte2 : public Directive {
  struct Config {
    QString name = u"WORD"_qs;
  };

  explicit Byte2();
  Byte2(QList<QSharedPointer<const argument::Base>> argument,
        FileLocation sourceLocation, QWeakPointer<Base> parent);
  Byte2(const Byte2 &other);
  Byte2(Byte2 &&other) noexcept;
  Byte2 &operator=(Byte2 other);
  friend void swap(Byte2 &first, Byte2 &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._config, second._config);
    swap(first._argument, second._argument);
    swap(first._emitsBytes, second._emitsBytes);
  }

  const Config &config() const;
  void setConfig(Config config);

  struct ValidateResult {
    bool valid = true;
    qsizetype argumentIndex = 0;
    QString errorMessage = {};
  };
  static ValidateResult
  validate_argument(const QList<QSharedPointer<const argument::Base>> argument);

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
  QList<QSharedPointer<const argument::Base>>
      _argument; // numeric && fixedSize, truncated to 1 byte.
  bool _emitsBytes = true;
};

} // namespace pat::ast::node
