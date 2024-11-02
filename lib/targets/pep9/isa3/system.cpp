/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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

#include "./system.hpp"
#include "link/bytes.hpp"
#include "link/memmap.hpp"
#include "link/mmio.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/readonly.hpp"
#include "sim/device/simple_bus.hpp"
#include "targets/pep9/isa3/cpu.hpp"
#include "targets/pep9/isa3/helpers.hpp"
using namespace Qt::StringLiterals;

using AddressSpan = sim::api2::memory::AddressSpan<quint16>;
namespace {
sim::api2::device::Descriptor desc_cpu(sim::api2::device::ID id) {
  return {.id = id, .baseName = "cpu", .fullName = "/cpu"};
}
sim::api2::device::Descriptor desc_bus(sim::api2::device::ID id) {
  return {.id = id, .baseName = "bus", .fullName = "/bus"};
}
sim::api2::device::Descriptor desc_dense(sim::api2::device::ID id) {
  return {.id = id, .baseName = u"dense%1"_s.arg(id), .fullName = u"/bus/dense%1"_s.arg(id)};
}
sim::api2::device::Descriptor desc_mmi(sim::api2::device::ID id, QString name) {
  return {.id = id, .baseName = u"mmi-%1"_s.arg(name), .fullName = u"/bus/mmi-%1"_s.arg(name)};
}
sim::api2::device::Descriptor desc_mmo(sim::api2::device::ID id, QString name) {
  return {.id = id, .baseName = u"mmo-%1"_s.arg(name), .fullName = u"/bus/mmo-%1"_s.arg(name)};
}
const auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};
} // namespace

targets::pep9::isa::System::System(QList<obj::MemoryRegion> regions, QList<obj::AddressedIO> mmios)
    : _regions(), _cpu(QSharedPointer<CPU>::create(desc_cpu(nextID()), _nextIDGenerator)),
      _bus(QSharedPointer<sim::memory::SimpleBus<quint16>>::create(desc_bus(nextID()), AddressSpan(0, 0xFFFF))),
      _paths(QSharedPointer<sim::api2::Paths>::create()) {
  addDevice(_cpu->device());
  addDevice(_cpu->csrs()->device());
  addDevice(_cpu->regs()->device());
  _bus->setPathManager(_paths);
  _paths->add(0, _bus->deviceID());
  // Construct Dense memory and ignore W bit, since we have no mechanism for it.
  for (const auto &reg : regions) {
    auto span = AddressSpan(0, static_cast<quint16>(reg.maxOffset - reg.minOffset));
    auto desc = desc_dense(nextID());
    addDevice(desc);
    auto mem = QSharedPointer<sim::memory::Dense<quint16>>::create(desc, span);
    _rawMemory.push_back(mem);
    sim::api2::memory::Target<quint16> *target = &*mem;
    if (!reg.w) {
      auto ro = QSharedPointer<sim::memory::ReadOnly<quint16>>::create(false);
      _readonly.push_back(ro);
      ro->setTarget(target, nullptr);
      target = &*ro;
    }
    _bus->pushFrontTarget(AddressSpan(reg.minOffset, reg.maxOffset), target);
    appendReloadEntries(mem, reg, static_cast<quint16>(-reg.minOffset));
    // Perform load!
    loadRegion(*mem, reg, static_cast<quint16>(-reg.minOffset));
  }

  // Create MMIO, do not perform buffering
  for (const auto &mmio : mmios) {
    auto span = AddressSpan(0, static_cast<quint16>(mmio.maxOffset - mmio.minOffset));
    if (mmio.type == obj::IO::Type::kInput) {
      auto desc = desc_mmi(nextID(), mmio.name);
      addDevice(desc);
      auto mem = QSharedPointer<sim::memory::Input<quint16>>::create(desc, span);
      _bus->pushFrontTarget(AddressSpan(mmio.minOffset, mmio.maxOffset), &*mem);
      _mmi[mmio.name] = mem;
      // By default, charIn should raise an error when it runs out of input.
      if (mmio.name == "charIn") mem->setFailPolicy(sim::api2::memory::FailPolicy::RaiseError);
      // Disk in must not raise an error, otherwise loader will not work.
      else if (mmio.name == "diskIn") {
        mem->setFailPolicy(sim::api2::memory::FailPolicy::YieldDefaultValue);
        mem->clear('z' /*Loader sentinel character*/);
      }
    } else if (mmio.type == obj::IO::Type::kOutput) {
      auto desc = desc_mmo(nextID(), mmio.name);
      addDevice(desc);
      auto mem = QSharedPointer<sim::memory::Output<quint16>>::create(desc, span);
      _bus->pushFrontTarget(AddressSpan(mmio.minOffset, mmio.maxOffset), &*mem);
      _mmo[mmio.name] = mem;
    } else {
      throw std::logic_error("Unreachable");
    }
  }
  _cpu->setTarget(&*_bus, nullptr);
}

std::pair<sim::api2::tick::Type, sim::api2::tick::Result>
targets::pep9::isa::System::tick(sim::api2::Scheduler::Mode mode) {
  auto res = _cpu->clock(_tick);
  return {++_tick, res};
}

sim::api2::tick::Type targets::pep9::isa::System::currentTick() const { return _tick; }

sim::api2::device::ID targets::pep9::isa::System::nextID() { return _nextID++; }

sim::api2::device::IDGenerator targets::pep9::isa::System::nextIDGenerator() { return _nextIDGenerator; }

void targets::pep9::isa::System::addDevice(sim::api2::device::Descriptor desc) { _devices[desc.id] = desc; }

sim::api2::device::Descriptor *targets::pep9::isa::System::descriptor(sim::api2::device::ID id) {
  if (auto it = _devices.find(id); it == _devices.cend()) return nullptr;
  else return &it.value();
}

void targets::pep9::isa::System::setBuffer(sim::api2::trace::Buffer *buffer) {
  static const char *const e = "Unimplemented";
  qCritical(e);
  throw std::logic_error(e);
}

QSharedPointer<const sim::api2::Paths> targets::pep9::isa::System::pathManager() const { return _paths; }

void targets::pep9::isa::System::setBootFlagAddress(quint16 addr) { _bootFlg = addr; }

void targets::pep9::isa::System::setBootFlags(bool enableLoader, bool enableDispatcher) {
  quint16 value = (enableLoader ? 1 << 0 : 0) | (enableDispatcher ? 1 << 1 : 0);
  if (bits::hostOrder() != bits::Order::BigEndian) value = bits::byteswap(value);
  if (_bootFlg) {
    _bus->write(*_bootFlg, {reinterpret_cast<quint8 *>(&value), 2}, gs);
  }
}

std::optional<quint16> targets::pep9::isa::System::getBootFlagAddress() { return _bootFlg; }

quint16 targets::pep9::isa::System::getBootFlags() const {
  quint8 buf[2];
  bits::span<quint8> bufSpan = {buf};
  bits::memclr(bufSpan);
  if (_bootFlg) {
    _bus->read(*_bootFlg, bufSpan, gs);
  }
  return bits::memcpy_endian<quint16>(bufSpan, bits::Order::BigEndian);
}

void targets::pep9::isa::System::init() {

  quint8 buf[2];
  bits::span<quint8> bufSpan = {buf};
  // Clear registers and CSRs before inserting non-0 values.
  cpu()->csrs()->clear(0);
  cpu()->regs()->clear(0);

  doReloadEntries();
  // In Pep/9, we cannot execute the OS followed by the user program.
  // By default, assume we are just executing the user program.
  _bus->read(static_cast<quint16>(00), bufSpan, gs);
  writeRegister(cpu()->regs(), ::isa::Pep9::Register::PC, bits::memcpy_endian<quint16>(bufSpan, bits::Order::BigEndian),
                gs);
  // Initalize SP to user stack pointer
  _bus->read(static_cast<quint16>(::isa::Pep9::MemoryVectors::UserStackPtr), bufSpan, gs);
  writeRegister(cpu()->regs(), ::isa::Pep9::Register::SP, bits::memcpy_endian<quint16>(bufSpan, bits::Order::BigEndian),
                gs);
  cpu()->updateStartingPC();
}

targets::pep9::isa::CPU *targets::pep9::isa::System::cpu() { return &*_cpu; }

sim::memory::SimpleBus<quint16> *targets::pep9::isa::System::bus() { return &*_bus; }

QStringList targets::pep9::isa::System::inputs() const { return _mmi.keys(); }

sim::memory::Input<quint16> *targets::pep9::isa::System::input(QString name) {
  if (auto find = _mmi.find(name); find != _mmi.end()) return &**find;
  return nullptr;
}

QStringList targets::pep9::isa::System::outputs() const { return _mmo.keys(); }

sim::memory::Output<quint16> *targets::pep9::isa::System::output(QString name) {
  if (auto find = _mmo.find(name); find != _mmo.end()) return &**find;
  return nullptr;
}

void targets::pep9::isa::System::doReloadEntries() {
  for (const auto &reg : _regions) {
    using size_type = bits::span<const quint8>::size_type;
    reg.target->write(reg.base,
                      {reinterpret_cast<const quint8 *>(reg.data.data()), static_cast<size_type>(reg.data.size())}, gs);
  }
}

void targets::pep9::isa::System::appendReloadEntries(QSharedPointer<sim::api2::memory::Target<quint16>> mem,
                                                     const obj::MemoryRegion &reg, quint16 baseOffset) {
  quint16 base = baseOffset + reg.minOffset;
  for (const auto seg : reg.segs) {
    auto fileData = seg->get_data();
    auto size = seg->get_file_size();
    if (fileData == nullptr) continue;
    std::vector<quint8> data(size);
    std::copy(fileData, fileData + size, data.data());
    _regions.push_back(ReloadHelper{.target = mem, .base = base, .data = std::move(data)});
    base += size;
  }
}

QSharedPointer<targets::pep9::isa::System> targets::pep9::isa::systemFromElf(const ELFIO::elfio &elf,
                                                                             bool loadUserImmediate) {
  using size_type = bits::span<const quint8>::size_type;
  auto segs = obj::getLoadableSegments(elf);
  auto memmap = obj::mergeSegmentRegions(segs);
  auto mmios = obj::getMMIODeclarations(elf);
  auto buffers = obj::getMMIBuffers(elf);

  auto ret = QSharedPointer<targets::pep9::isa::System>::create(memmap, mmios);

  // Either immediately load user program into memory, or buffer behind correct
  // port.
  if (loadUserImmediate) {
    quint16 address = 0;
    auto bus = ret->bus();
    for (auto buffer : buffers) {
      auto ptr = reinterpret_cast<const quint8 *>(buffer.seg->get_data());
      if (ptr == nullptr) continue;
      const auto ret = bus->write(address, {ptr, static_cast<size_type>(buffer.seg->get_memory_size())}, gs);
      address += buffer.seg->get_memory_size();
    }
  } else {
    for (auto buffer : buffers) {
      auto mmi = ret->input(buffer.portName);
      Q_ASSERT(mmi != nullptr);
      auto endpoint = mmi->endpoint();
      auto bytesAsHex = obj::segmentAsAsciiHex(buffer.seg);
      /*std::cout << "[SGLD]<";
      std::cout.write((char *)bytesAsHex.data(), bytesAsHex.size());
      std::cout << std::endl;*/
      for (auto byte : bytesAsHex) endpoint->append_value(byte);
    }

    auto diskIn = ret->input("diskIn");
    Q_ASSERT(diskIn != nullptr);
    auto endpoint = diskIn->endpoint();
    endpoint->append_value(' ');
    endpoint->append_value('z');
    endpoint->append_value('z');
  }

  if (auto bootFlg = obj::getBootFlagsAddress(elf); bootFlg) ret->setBootFlagAddress(*bootFlg);

  return ret;
}

bool targets::pep9::isa::loadRegion(sim::api2::memory::Target<quint16> &mem, const obj::MemoryRegion &reg,
                                    quint16 baseOffset) {
  constexpr auto gs = sim::api2::memory::Operation{
      .type = sim::api2::memory::Operation::Type::Application,
      .kind = sim::api2::memory::Operation::Kind::data,
  };
  auto ret = true;
  quint16 base = baseOffset + reg.minOffset;
  for (const auto seg : reg.segs) {
    auto fileData = seg->get_data();
    auto size = seg->get_file_size();
    if (fileData == nullptr) continue;
    using size_type = bits::span<const quint8>::size_type;
    mem.write(base, {reinterpret_cast<const quint8 *>(fileData), static_cast<size_type>(size)}, gs);
    base += size;
  }
  return ret;
}

bool targets::pep9::isa::loadElfSegments(sim::api2::memory::Target<quint16> &mem, const ELFIO::elfio &elf) {
  const auto gs = sim::api2::memory::Operation{
      .type = sim::api2::memory::Operation::Type::Application,
      .kind = sim::api2::memory::Operation::Kind::data,
  };

  using size_type = bits::span<const quint8>::size_type;
  auto segs = obj::getLoadableSegments(elf);
  auto memmap = obj::mergeSegmentRegions(segs);
  bool ret = true;
  for (const auto &reg : memmap) {
    ret &= loadRegion(mem, reg, 0);
  }
  return ret;
}
