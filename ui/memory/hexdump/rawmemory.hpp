#pragma once
#include <QObject>
#include <QtTypes>
#include <vector>
#include "../memory_globals.hpp"
#include "sim/device/simple_bus.hpp"
#include "sim/trace2/modified.hpp"

class QJSEngine;
class QQmlEngine;
class MEMORY_EXPORT MemoryHighlight : public QObject {
  Q_OBJECT
public:
  enum V {
    None,
    Modified,
    SP,
    PC,
  };
  Q_ENUM(V)
};
class MEMORY_EXPORT ARawMemory : public QObject {
  Q_OBJECT
public:
  ARawMemory(QObject *parent = nullptr);
  virtual ~ARawMemory() = 0;
  virtual quint32 byteCount() const = 0;
  virtual quint8 read(quint32 address) const = 0;
  virtual MemoryHighlight::V status(quint32 address) const;
  virtual void write(quint32 address, quint8 value) = 0;
  virtual void clear() = 0;
};

class MEMORY_EXPORT EmptyRawMemory : public ARawMemory {
  Q_OBJECT
public:
  explicit EmptyRawMemory(quint32 size, QObject *parent = nullptr);
  quint32 byteCount() const override;
  quint8 read(quint32 address) const override;
  void write(quint32 address, quint8 value) override;
  void clear() override;

private:
  quint32 _size;
};

class MEMORY_EXPORT EmptyRawMemoryFactory : public QObject {
  Q_OBJECT

public:
  Q_INVOKABLE EmptyRawMemory *create(quint32 size);
  static QObject *singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine);
};

class MEMORY_EXPORT ArrayRawMemory : public ARawMemory {
  Q_OBJECT
public:
  explicit ArrayRawMemory(quint32 size, QObject *parent = nullptr);
  quint32 byteCount() const override;
  quint8 read(quint32 address) const override;
  MemoryHighlight::V status(quint32 address) const override;
  void write(quint32 address, quint8 value) override;
  void clear() override;

private:
  std::vector<quint8> _data;
};

class MEMORY_EXPORT ArrayRawMemoryFactory : public QObject {
  Q_OBJECT
public:
  Q_INVOKABLE ArrayRawMemory *create(quint32 size);
  static QObject *singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine);
};

namespace sim::trace2 {
template <typename T> class ModifiedAddressSink;
}
// TODO: add access to CPU, add access to traces.
class MEMORY_EXPORT SimulatorRawMemory : public ARawMemory {
  Q_OBJECT
public:
  explicit SimulatorRawMemory(sim::memory::SimpleBus<quint16> *memory,
                              QSharedPointer<sim::trace2::ModifiedAddressSink<quint16>> addrSink,
                              QObject *parent = nullptr);
  quint32 byteCount() const override;
  quint8 read(quint32 address) const override;
  void setPC(quint32 start, quint32 end);
  void setSP(quint32 address);
  MemoryHighlight::V status(quint32 address) const override;
  void write(quint32 address, quint8 value) override;
  void clear() override;

private:
  sim::memory::SimpleBus<quint16> *_memory;
  QSharedPointer<sim::trace2::ModifiedAddressSink<quint16>> _sink;
  static constexpr quint32 n1 = -1;
  sim::trace2::Interval<quint32> _pc = {n1, n1}, _sp = {n1, n1};
};
