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
};
}; // namespace pat::ast
