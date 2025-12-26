/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <QtCore>
struct RegisterFormatter {
  virtual ~RegisterFormatter() = default;
  virtual QString format() const = 0;
  virtual QString format(quint8 byteCount) const = 0;
  virtual bool readOnly() const = 0;
  virtual qsizetype length() const = 0;
  virtual qsizetype length(quint8 byteCount) const = 0;
  virtual QString describe() const = 0;
};

struct TextFormatter : public RegisterFormatter {
  explicit TextFormatter(QString value);
  ~TextFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;
  QString describe() const override { return "Text"; }

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
  QString describe() const override { return "Hexadecimal"; }

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
  QString describe() const override { return "Unsigned Decimal"; }

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
  QString describe() const override { return "Signed Decimal"; }

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
  QString describe() const override { return "Binary"; }

private:
  uint16_t _len = 0;
  uint64_t _mask = 0;
  std::function<uint64_t()> _fn;
};

struct ASCIIFormatter : public RegisterFormatter {
  explicit ASCIIFormatter(std::function<uint64_t()> fn, uint16_t byteCount = 1);
  ~ASCIIFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;
  QString describe() const override { return "ASCII"; }

private:
  uint16_t _bytes = 0;
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
  QString describe() const override { return "Mnemonic"; }

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
  QString describe() const override { return _fmt->describe(); }

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
  QString describe() const override { return _fmt->describe(); }

private:
  quint8 _maxBytes = 2;
  QSharedPointer<RegisterFormatter> _fmt;
  std::function<quint8()> _bytes;
};

struct AutoChoice : public RegisterFormatter {
  explicit AutoChoice(QVector<QSharedPointer<RegisterFormatter>> formatters, std::function<qsizetype()> choice);
  ~AutoChoice() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;
  QString describe() const override;
  const RegisterFormatter *current() const;

private:
  QVector<QSharedPointer<RegisterFormatter>> _formatters;
  std::function<qsizetype()> _currentChoice;
};

struct ChoiceFormatter : public RegisterFormatter {
  explicit ChoiceFormatter(QVector<QSharedPointer<RegisterFormatter>> formatters, qsizetype currentChoice = 0);
  ~ChoiceFormatter() override = default;
  QString format() const override;
  QString format(quint8 byteCount) const override;
  bool readOnly() const override;
  qsizetype length() const override;
  qsizetype length(quint8 byteCount) const override;
  QStringList choices() const;
  qsizetype currentIndex() const;
  bool setCurrentIndex(qsizetype index);
  const RegisterFormatter *current() const;

  QString describe() const override { return _formatters[_currentChoice]->describe(); }

private:
  QVector<QSharedPointer<RegisterFormatter>> _formatters;
  qsizetype _currentChoice;
};
