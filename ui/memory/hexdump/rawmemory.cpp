#include "rawmemory.hpp"

#include <QQmlEngine>

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

QObject *EmptyRawMemoryFactory::singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  return new EmptyRawMemoryFactory();
}

ArrayRawMemory::ArrayRawMemory(quint32 size, QObject *parent) : ARawMemory(parent), _data(size, 0) {}

quint32 ArrayRawMemory::byteCount() const { return _data.size(); }

quint8 ArrayRawMemory::read(quint32 address) const { return _data[address]; }

MemoryHighlight::V ArrayRawMemory::status(quint32 address) const {
  using V = MemoryHighlight::V;
  if (address % 32 == 0)
    return V::PC;
  else if (address % 16 == 0)
    return V::SP;
  else if (address % 8 == 0)
    return V::Modified;
  else
    return V::None;
}

void ArrayRawMemory::write(quint32 address, quint8 value) { _data[address] = value; }

void ArrayRawMemory::clear() { std::fill(_data.begin(), _data.end(), 0); }

ArrayRawMemory *ArrayRawMemoryFactory::create(quint32 size) {
  auto ret = new ArrayRawMemory(size);
  // Should be implicitly set due to returning a raw pointer, but be explicit for documentation purposes.
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::JavaScriptOwnership);
  return ret;
}

QObject *ArrayRawMemoryFactory::singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  return new ArrayRawMemoryFactory();
}

static const auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};

SimulatorRawMemory::SimulatorRawMemory(sim::memory::SimpleBus<quint16> *memory,
                                       QSharedPointer<sim::trace2::ModifiedAddressSink<quint16>> addrSink,
                                       QObject *parent)
    : ARawMemory(parent), _memory(memory), _sink(addrSink) {}

quint32 SimulatorRawMemory::byteCount() const {
  auto span = _memory->span();
  return 1 + span.maxOffset - span.minOffset;
}

quint8 SimulatorRawMemory::read(quint32 address) const {
  auto span = _memory->span();
  quint8 ret = 0;
  if (address >= span.minOffset && address <= span.maxOffset) {
    _memory->read(address, {&ret, sizeof(ret)}, gs);
  }
  return ret;
}

void SimulatorRawMemory::setPC(quint32 start, quint32 end) { _pc = {start, end}; }

void SimulatorRawMemory::setSP(quint32 address) { _sp = {address, address}; }

MemoryHighlight::V SimulatorRawMemory::status(quint32 address) const {
  if (sim::trace2::contains(_pc, address))
    return MemoryHighlight::PC;
  else if (sim::trace2::contains(_sp, address))
    return MemoryHighlight::SP;
  else if (!_sink.isNull() && _sink->contains(address))
    return MemoryHighlight::Modified;
  return MemoryHighlight::None;
}

void SimulatorRawMemory::write(quint32 address, quint8 value) {
  auto span = _memory->span();
  if (address >= span.minOffset && address <= span.maxOffset) {
    _memory->write(address, {&value, sizeof(value)}, gs);
  }
}

void SimulatorRawMemory::clear() { _memory->clear(0); }
