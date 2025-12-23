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
#include "rawmemory.hpp"

#include <QQmlEngine>

#include <sim/trace2/modified.hpp>

ARawMemory::ARawMemory(QObject *parent) : QObject(parent) {}

MemoryHighlight::V ARawMemory::status(quint32 address) const { return MemoryHighlight::None; }

ARawMemory::~ARawMemory() = default;

EmptyRawMemory::EmptyRawMemory(quint32 size, QObject *parent) : ARawMemory(parent), _size(size) {}

quint32 EmptyRawMemory::byteCount() const { return _size; }

quint8 EmptyRawMemory::read(quint32 address) const { return 0; }

void EmptyRawMemory::write(quint32 address, quint8 value) {}

void EmptyRawMemory::clear() {}

EmptyRawMemory *EmptyRawMemoryFactory::create(quint32 size) {
  auto ret = new EmptyRawMemory(size);
  // Should be implicitly set due to returning a raw pointer, but be explicit for documentation purposes.
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::JavaScriptOwnership);
  return ret;
}

QObject *EmptyRawMemoryFactory::create(QQmlEngine *, QJSEngine *) { return new EmptyRawMemoryFactory(); }

ArrayRawMemory::ArrayRawMemory(quint32 size, QObject *parent) : ARawMemory(parent), _data(size, 0) {}

quint32 ArrayRawMemory::byteCount() const { return _data.size(); }

quint8 ArrayRawMemory::read(quint32 address) const { return _data[address]; }

MemoryHighlight::V ArrayRawMemory::status(quint32 address) const {
  using V = MemoryHighlight::V;
  if (address % 32 == 0) return V::PC;
  else if (address % 16 == 0) return V::SP;
  else if (address % 8 == 0) return V::Modified;
  else return V::None;
}

void ArrayRawMemory::write(quint32 address, quint8 value) { _data[address] = value; }

void ArrayRawMemory::clear() { std::fill(_data.begin(), _data.end(), 0); }

ArrayRawMemory *ArrayRawMemoryFactory::create(quint32 size) {
  auto ret = new ArrayRawMemory(size);
  // Should be implicitly set due to returning a raw pointer, but be explicit for documentation purposes.
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::JavaScriptOwnership);
  return ret;
}

QObject *ArrayRawMemoryFactory::create(QQmlEngine *, QJSEngine *) { return new ArrayRawMemoryFactory(); }

static const auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};

SimulatorRawMemory::SimulatorRawMemory(sim::memory::SimpleBus<quint16> *memory,
                                       QSharedPointer<sim::trace2::ModifiedAddressSink<quint16>> addrSink,
                                       QObject *parent)
    : ARawMemory(parent), _memory(memory), _sink(addrSink) {}

quint32 SimulatorRawMemory::byteCount() const { return sim::api2::memory::size<quint16, false>(_memory->span()); }

quint8 SimulatorRawMemory::read(quint32 address) const {
  auto span = _memory->span();
  quint8 ret = 0;
  if (sim::api2::memory::contains(span, static_cast<quint16>(address))) _memory->read(address, {&ret, sizeof(ret)}, gs);
  return ret;
}

std::optional<quint8> SimulatorRawMemory::readPrevious(quint32 address) const {
  auto it = _modifiedCache.find(address);
  if (it != _modifiedCache.end()) return it->second;
  return std::nullopt;
}

void SimulatorRawMemory::setPC(quint32 start, quint32 end) { _PC = {start, end}; }

void SimulatorRawMemory::setSP(quint32 address) { _SP = {address, address}; }

quint32 SimulatorRawMemory::pc() const { return _PC.lower(); }

quint32 SimulatorRawMemory::sp() const { return _SP.lower(); }

MemoryHighlight::V SimulatorRawMemory::status(quint32 address) const {
  using sim::api2::memory::contains;
  if (contains(_PC, address)) return MemoryHighlight::PC;
  else if (contains(_SP, address)) return MemoryHighlight::SP;
  else if (!_sink.isNull() && _sink->contains(address)) return MemoryHighlight::Modified;
  return MemoryHighlight::None;
}

void SimulatorRawMemory::write(quint32 address, quint8 value) {
  using sim::api2::memory::contains;
  auto span = _memory->span();
  if (contains(span, static_cast<quint16>(address))) _memory->write(address, {&value, sizeof(value)}, gs);
}

void SimulatorRawMemory::clear() {
  _memory->clear(0);
  _PC = _lastPC = _SP = _lastSP = {n1, n1};
}

void SimulatorRawMemory::clearModifiedAndUpdateGUI() {
  _sink->clear();
  _modifiedCache.clear();
  emit dataChanged(0, 0xffff);
}

void SimulatorRawMemory::onUpdateGUI(sim::api2::trace::FrameIterator from) {
  // Remove highlighted cells from previous steps.
  auto oldHighlights = std::set{_sink->intervals()};
  oldHighlights.insert({static_cast<quint16>(_lastSP.lower()), static_cast<quint16>(_lastSP.upper())});
  oldHighlights.insert({static_cast<quint16>(_lastPC.lower()), static_cast<quint16>(_lastPC.upper())});
  // Purge data from previous updates. Must be cleared before iterating and emitting events, or highlights are wrong.
  _sink->clear();
  // Must cache current SP/PC so that we can clear the highlighting next time.
  _lastSP = _SP, _lastPC = _PC;

  // If there is no TB, then there is no data to analyze.
  // Conservatively, we assume that all data is modified.
  if (const auto tb = _memory->buffer(); tb == nullptr) emit dataChanged(0, 0xffff);
  else {
    for (auto oldHighlight : oldHighlights) emit dataChanged(oldHighlight.lower(), oldHighlight.upper());
    for (auto frame = from; frame != tb->cend(); ++frame)
      for (auto packet = frame.cbegin(); packet != frame.cend(); ++packet)
        _sink->analyze(packet, sim::api2::trace::Direction::Forward);
    for (const auto &interval : _sink->intervals())
      // Cache the previous value of the modified addresses.
      emit dataChanged(interval.lower(), interval.upper());
    // And update intervals containing PC, SP to fix the highlighting.
    emit dataChanged(this->_SP.lower(), this->_SP.upper());
    emit dataChanged(this->_PC.lower(), this->_PC.upper());
  }
}

void SimulatorRawMemory::onRepaintAddress(quint32 start, quint32 end) { emit dataChanged(start, end); }
