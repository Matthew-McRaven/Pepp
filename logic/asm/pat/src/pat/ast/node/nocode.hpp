#pragma once
#include "./base.hpp"
#include <QtCore>
namespace pat::ast::node {
class Blank : public node::Base {
public:
  explicit Blank();
  Blank(FileLocation sourceLocation, QWeakPointer<node::Base> parent);
  Blank(const Blank &other);
  Blank(Blank &&other) noexcept;
  Blank &operator=(Blank other);
  friend void swap(Blank &first, Blank &second) {
    using std::swap;
    swap((Base &)first, (Base &)second);
  }

  // Value interface
  QSharedPointer<Value> clone() const override;
  bits::BitOrder endian() const override;
  quint64 size() const override;
  bool bits(QByteArray &out, bits::BitSelection src,
            bits::BitSelection dest) const override;
  bool bytes(QByteArray &out, qsizetype start, qsizetype length) const override;
  QString string() const override;

  // Base interface
  const AddressSpan &addressSpan() const override;
  void updateAddressSpan(void *update) const override;
  bool emitsBytes() const override;
  void setEmitsBytes(bool emitBytes) override;
};

class Comment : public node::Base {
public:
  struct Config {
    QString commentExpr = u";"_qs;
  };
  enum class IndentLevel {
    Left,
    Instruction,
  };
  explicit Comment();
  Comment(QString comment, FileLocation sourceLocation,
          QWeakPointer<node::Base> parent);
  Comment(const Comment &other);
  Comment(Comment &&other) noexcept;
  Comment &operator=(Comment other);
  friend void swap(Comment &first, Comment &second) {
    using std::swap;
    swap((Base &)first, (Base &)second);
    swap(first._config, second._config);
    swap(first._indent, second._indent);
    swap(first._comment, second._comment);
  }

  const Config &config() const;
  void setConfig(Config config);
  IndentLevel indent() const;
  void setIndent(IndentLevel indent);

  // Value interface
  QSharedPointer<Value> clone() const override;
  bits::BitOrder endian() const override;
  quint64 size() const override;
  bool bits(QByteArray &out, bits::BitSelection src,
            bits::BitSelection dest) const override;
  bool bytes(QByteArray &out, qsizetype start, qsizetype length) const override;
  QString string() const override;

  // Base interface
  const AddressSpan &addressSpan() const override;
  void updateAddressSpan(void *update) const override;
  bool emitsBytes() const override;
  void setEmitsBytes(bool emitBytes) override;

private:
  Config _config = {};
  IndentLevel _indent = IndentLevel::Instruction;
  QString _comment = {};
};
} // namespace pat::ast::node
