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
#include <QObject>
#include <QtQmlIntegration>
#include <QtTypes>
#include <vector>
#include "sim3/subsystems/bus/simple.hpp"
#include "sim3/trace/modified.hpp"

class QJSEngine;
class QQmlEngine;
class MemoryHighlight : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")

public:
  enum V {
    None,
    Modified,
    SP,
    PC,
  };
  Q_ENUM(V)
};
class ARawMemory : public QObject {
  Q_OBJECT

public:
  ARawMemory(QObject *parent = nullptr);
  virtual ~ARawMemory() = 0;
  virtual quint32 byteCount() const = 0;
  virtual quint8 read(quint32 address) const = 0;
  virtual std::optional<quint8> readPrevious(quint32 address) const { return std::nullopt; }
  virtual MemoryHighlight::V status(quint32 address) const;
  virtual void write(quint32 address, quint8 value) = 0;
  virtual void clear() = 0;
  Q_INVOKABLE virtual quint32 pc() const { return 0; }
  Q_INVOKABLE virtual quint32 sp() const { return 0; }
signals:
  void dataChanged(quint32 start, quint32 end);
};

class EmptyRawMemory : public ARawMemory {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")

public:
  explicit EmptyRawMemory(quint32 size, QObject *parent = nullptr);
  quint32 byteCount() const override;
  quint8 read(quint32 address) const override;
  void write(quint32 address, quint8 value) override;
  void clear() override;

private:
  quint32 _size;
};

class EmptyRawMemoryFactory : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE EmptyRawMemory *create(quint32 size);
  static QObject *create(QQmlEngine *, QJSEngine *);
};

class ArrayRawMemory : public ARawMemory {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")

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

class ArrayRawMemoryFactory : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE ArrayRawMemory *create(quint32 size);
  static QObject *create(QQmlEngine *, QJSEngine *);
};

namespace sim::trace2 {
template <typename T> class ModifiedAddressSink;
}
// TODO: add access to CPU, add access to traces.
class SimulatorRawMemory : public ARawMemory {
  Q_OBJECT
public:
  explicit SimulatorRawMemory(sim::memory::SimpleBus<quint16> *memory,
                              QSharedPointer<sim::trace2::ModifiedAddressSink<quint16>> addrSink,
                              QObject *parent = nullptr);
  quint32 byteCount() const override;
  quint8 read(quint32 address) const override;
  std::optional<quint8> readPrevious(quint32 address) const override;
  void setPC(quint32 start, quint32 end);
  void setSP(quint32 address);
  Q_INVOKABLE quint32 pc() const override;
  Q_INVOKABLE quint32 sp() const override;
  MemoryHighlight::V status(quint32 address) const override;
  void write(quint32 address, quint8 value) override;
  void clear() override;
public slots:
  void clearModifiedAndUpdateGUI();
  void onUpdateGUI(sim::api2::trace::FrameIterator from);
  // Addresses were changed, but not tracked in the trace buffer.
  // We don't want to highlight them. We just want to make sure they get re-painted.
  void onRepaintAddress(quint32 start, quint32 end);

private:
  sim::memory::SimpleBus<quint16> *_memory;
  std::map<quint32, quint8> _modifiedCache;
  QSharedPointer<sim::trace2::ModifiedAddressSink<quint16>> _sink;
  static constexpr quint32 n1 = -1;
  sim::trace2::Interval<quint32> _PC = {n1, n1}, _SP = {n1, n1};
  sim::trace2::Interval<quint32> _lastPC = {n1, n1}, _lastSP = {n1, n1};
};
