#pragma once
#include "../bits/order.hpp"
#include "../bits/select.hpp"
#include <QtCore>
namespace pat::ast {
struct Value {
  virtual QSharedPointer<Value> clone() const = 0;
  virtual bits::BitOrder endian() const = 0;
  virtual quint64 size() const = 0;
  virtual bool bits(QByteArray &out, bits::BitSelection src,
                    bits::BitSelection dest) const = 0;
  virtual bool bytes(QByteArray &out, qsizetype start,
                     qsizetype length) const = 0;
  virtual QString string() const = 0;
  friend void swap(Value &first, Value &second) { using std::swap; }

protected:
  explicit Value() = default;
  Value(const Value &other) = delete;
  Value &operator=(const Value &other) = delete;
};
}; // namespace pat::ast
