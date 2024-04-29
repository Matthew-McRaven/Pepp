#include "rawmemory.hpp"

ARawMemory::ARawMemory(QObject *parent) : QObject(parent) {}

ARawMemory::~ARawMemory() = default;

EmptyRawMemory::EmptyRawMemory(quint32 size, QObject *parent) : ARawMemory(parent), _size(size) {}

quint32 EmptyRawMemory::byteCount() const { return _size; }

quint8 EmptyRawMemory::read(quint32 address) const { return 0; }

void EmptyRawMemory::write(quint32 address, quint8 value) {}

void EmptyRawMemory::clear() {}

TableRawMemory::TableRawMemory(quint32 size, QObject *parent) : ARawMemory(parent), _data(size, 0) {}

quint32 TableRawMemory::byteCount() const { return _data.size(); }

quint8 TableRawMemory::read(quint32 address) const { return _data[address]; }

void TableRawMemory::write(quint32 address, quint8 value) { _data[address] = value; }

void TableRawMemory::clear() { std::fill(_data.begin(), _data.end(), 0); }
