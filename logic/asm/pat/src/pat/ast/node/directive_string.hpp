#pragma once
#include "./directive.hpp"

namespace pat::ast::node {
struct ASCII : public Directive {
  struct Config {
    QString name = u"ASCII"_qs;
    bool nullTerminate = false;
  };

  explicit ASCII();
  ASCII(QSharedPointer<argument::Base> argument, FileLocation sourceLocation,
        QWeakPointer<node::Base> parent);
  ASCII(const ASCII &other);
  ASCII(ASCII &&other) noexcept;
  ASCII &operator=(ASCII other);
  friend void swap(ASCII &first, ASCII &second) {
    using std::swap;
    swap((Directive &)first, (Directive &)second);
    swap(first._config, second._config);
    swap(first._comment, second._comment);
    swap(first._emitsBytes, second._emitsBytes);
  }

  const Config &config() const;
  void setConfig(Config config);

  struct ValidateResult {
    bool valid = true;
    QString errorMessage = {};
  };
  static ValidateResult
  validate_argument(QSharedPointer<const argument::Base> argument);

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
  QSharedPointer<argument::Base> _argument = nullptr; // numeric && fixedSize
  bool _emitsBytes = true;
};
} // namespace pat::ast::node
