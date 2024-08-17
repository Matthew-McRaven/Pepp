#pragma once
#include <QtCore>
struct RegisterFormatter {
  virtual ~RegisterFormatter() = default;
  virtual QString format() const = 0;
  virtual QString format(quint8 byteCount) const = 0;
  virtual bool readOnly() const = 0;
  virtual qsizetype length() const = 0;
  virtual qsizetype length(quint8 byteCount) const = 0;
};

struct TextFormatter : public RegisterFormatter {
  explicit TextFormatter(QString value);
  ~TextFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;

private:
  QString _value;
};

struct HexFormatter : public RegisterFormatter {
  explicit HexFormatter(std::function<uint64_t()> fn, uint16_t byteCount = 2);
  ~HexFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;

private:
  uint16_t _bytes = 0;
  uint64_t _mask = 0;
  std::function<uint64_t()> _fn;
};

struct UnsignedDecFormatter : public RegisterFormatter {
  explicit UnsignedDecFormatter(std::function<std::uint64_t()> fn, uint16_t byteCount = 2);
  ~UnsignedDecFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;

private:
  static uint16_t digits(quint8 byteCount);
  uint16_t _bytes = 0, _len = 0;
  uint64_t _mask = 0;
  std::function<uint64_t()> _fn;
};

struct SignedDecFormatter : public RegisterFormatter {
  explicit SignedDecFormatter(std::function<int64_t()> fn, uint16_t byteCount = 2);
  ~SignedDecFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;

private:
  static uint16_t digits(quint8 byteCount);
  uint16_t _bytes = 0, _len = 0;
  uint64_t _mask = 0;
  std::function<int64_t()> _fn;
};

struct BinaryFormatter : public RegisterFormatter {
  explicit BinaryFormatter(std::function<uint64_t()> fn, uint16_t byteCount = 1);
  ~BinaryFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;

private:
  uint16_t _len = 0;
  uint64_t _mask = 0;
  std::function<uint64_t()> _fn;
};

// Ignore lengthed overrides since they do not depend on byte count
struct MnemonicFormatter : public RegisterFormatter {
  explicit MnemonicFormatter(std::function<QString()> fn);
  ~MnemonicFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;

private:
  std::function<QString()> _fn;
};

struct OptionalFormatter : public RegisterFormatter {
  explicit OptionalFormatter(QSharedPointer<RegisterFormatter> fmt, std::function<bool()> valid);
  ~OptionalFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;

private:
  QSharedPointer<RegisterFormatter> _fmt;
  std::function<bool()> _valid;
};

struct VariableByteLengthFormatter : public RegisterFormatter {
  explicit VariableByteLengthFormatter(QSharedPointer<RegisterFormatter> fmt, std::function<quint8()> bytes,
                                       quint8 maxBytes = 2);
  ~VariableByteLengthFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  // This method will be called to determine max column width, so we should always use the max.
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;

private:
  quint8 _maxBytes = 2;
  QSharedPointer<RegisterFormatter> _fmt;
  std::function<quint8()> _bytes;
};
