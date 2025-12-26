/*
 * /Copyright (c) 2023-2025. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "../cores/pep/traced_helpers.hpp"
#include "../cores/pep/traced_pep10_isa3.hpp"
#include "../cores/pep/traced_pep9_isa3.hpp"
#include "sim3/api/memory_address.hpp"
#include "sim3/subsystems/bus/simple.hpp"
#include "sim3/subsystems/disk/ide.hpp"
#include "sim3/subsystems/ram/broadcast/mmi.hpp"
#include "sim3/subsystems/ram/broadcast/mmo.hpp"
#include "sim3/subsystems/ram/readonly.hpp"
#include "toolchain/link/bytes.hpp"
#include "toolchain/link/memmap.hpp"
#include "toolchain/link/mmio.hpp"
#include "traced_pep_isa3_system.hpp"
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
sim::api2::device::Descriptor desc_ide(sim::api2::device::ID id, QString name) {
  return {.id = id, .baseName = u"ide-%1"_s.arg(name), .fullName = u"/bus/ide-%1"_s.arg(name)};
}
const auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};

QSharedPointer<sim::api2::tick::Recipient> create_cpu(pepp::Architecture arch, sim::api2::device::Descriptor desc,
                                                      sim::api2::device::IDGenerator gen) {
  using enum pepp::Architecture;
  switch (arch) {
  case PEP9: return QSharedPointer<targets::pep9::isa::CPU>::create(desc, gen);
  case PEP10: return QSharedPointer<targets::pep10::isa::CPU>::create(desc, gen);
  default: throw std::logic_error("Unimplemented");
  }
}
} // namespace

targets::isa::System::System(pepp::Architecture arch, QList<obj::MemoryRegion> regions, QList<obj::AddressedIO> mmios)
    : _regions(), _arch(arch), _cpu(create_cpu(arch, desc_cpu(nextID()), _nextIDGenerator)),
      _bus(QSharedPointer<sim::memory::SimpleBus<quint16>>::create(desc_bus(nextID()), AddressSpan(0, 0xFFFF))),
      _paths(QSharedPointer<sim::api2::Paths>::create()) {

  _bus->setPathManager(_paths);
  reconfigure(arch, regions, mmios);
}

std::pair<sim::api2::tick::Type, sim::api2::tick::Result> targets::isa::System::tick(sim::api2::Scheduler::Mode mode) {
  auto tb = _bus->buffer();
  // TODO: only emit frames if something changed this cycle
  if (tb) tb->emitFrameStart();
  auto res = _cpu->clock(_tick);
  if (tb) tb->updateFrameHeader();
  return {++_tick, res};
}

sim::api2::tick::Type targets::isa::System::currentTick() const { return _tick; }

sim::api2::device::ID targets::isa::System::nextID() { return _nextID++; }

sim::api2::device::IDGenerator targets::isa::System::nextIDGenerator() { return _nextIDGenerator; }

void targets::isa::System::addDevice(sim::api2::device::Descriptor desc) { _devices[desc.id] = desc; }

sim::api2::device::Descriptor *targets::isa::System::descriptor(sim::api2::device::ID id) {
  if (auto it = _devices.find(id); it == _devices.cend()) return nullptr;
  else return &it.value();
}

void targets::isa::System::setBuffer(sim::api2::trace::Buffer *buffer) {
  static const char *const e = "Unimplemented";
  qCritical(e);
  throw std::logic_error(e);
}

QSharedPointer<const sim::api2::Paths> targets::isa::System::pathManager() const { return _paths; }

void targets::isa::System::init() {
  using enum pepp::Architecture;
  quint8 buf[2];
  bits::span<quint8> bufSpan = {buf};
  // Reload default values into DDR.
  doReloadEntries();
  // 1. Clear registers and CSRs before inserting non-0 values.
  // 2. Initalize PC & SP.
  // 3. Update cached initial PC.
  // Use braces to limit scope of variables in switch.
  switch (_arch) {
  case PEP9: {
    using ISA = ::isa::Pep9;
    targets::pep9::isa::CPU *cpu = dynamic_cast<targets::pep9::isa::CPU *>(_cpu.data());
    auto regs = cpu->regs(), csrs = cpu->csrs();
    regs->clear(0), csrs->clear(0);
    bufSpan[0] = bufSpan[1] = 0;
    writeRegister<ISA>(regs, ISA::Register::PC, bits::memcpy_endian<quint16>(bufSpan, bits::Order::BigEndian), gs);
    _bus->read(static_cast<quint16>(ISA::MemoryVectors::UserStackPtr), bufSpan, gs);
    writeRegister<ISA>(regs, ISA::Register::SP, bits::memcpy_endian<quint16>(bufSpan, bits::Order::BigEndian), gs);
    cpu->updateStartingPC();
    break;
  }
  case PEP10: {
    using ISA = ::isa::Pep10;
    targets::pep10::isa::CPU *cpu = dynamic_cast<targets::pep10::isa::CPU *>(_cpu.data());
    auto regs = cpu->regs(), csrs = cpu->csrs();
    regs->clear(0), csrs->clear(0);
    _bus->read(static_cast<quint16>(ISA::MemoryVectors::Dispatcher), bufSpan, gs);
    writeRegister<ISA>(regs, ISA::Register::PC, bits::memcpy_endian<quint16>(bufSpan, bits::Order::BigEndian), gs);
    _bus->read(static_cast<quint16>(ISA::MemoryVectors::SystemStackPtr), bufSpan, gs);
    writeRegister<ISA>(regs, ISA::Register::SP, bits::memcpy_endian<quint16>(bufSpan, bits::Order::BigEndian), gs);
    cpu->updateStartingPC();
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }
}

pepp::Architecture targets::isa::System::architecture() const { return _arch; }

sim::api2::tick::Recipient *targets::isa::System::cpu() { return &*_cpu; }

sim::memory::SimpleBus<quint16> *targets::isa::System::bus() { return &*_bus; }

QStringList targets::isa::System::inputs() const { return _mmi.keys(); }

sim::memory::Input<quint16> *targets::isa::System::input(QString name) {
  if (auto find = _mmi.find(name); find != _mmi.end()) return &**find;
  return nullptr;
}

QStringList targets::isa::System::outputs() const { return _mmo.keys(); }

sim::memory::Output<quint16> *targets::isa::System::output(QString name) {
  if (auto find = _mmo.find(name); find != _mmo.end()) return &**find;
  return nullptr;
}

QStringList targets::isa::System::ideControllers() const { return _ide.keys(); }

sim::memory::IDEController *targets::isa::System::ideController(QString name) {
  if (auto find = _ide.find(name); find != _ide.end()) return &**find;
  return nullptr;
}

void targets::isa::System::doReloadEntries() {
  for (const auto &reg : _regions) {
    using size_type = bits::span<const quint8>::size_type;
    reg.target->write(reg.base,
                      {reinterpret_cast<const quint8 *>(reg.data.data()), static_cast<size_type>(reg.data.size())}, gs);
  }
}

// Duplicated logic from systemFromElf to get the params to pass to reconfigure.
void targets::isa::System::reconfigure(const ELFIO::elfio &elf) {

  using size_type = bits::span<const quint8>::size_type;
  auto segs = obj::getLoadableSegments(elf);
  auto memmap = obj::mergeSegmentRegions(segs);
  auto mmios = obj::getMMIODeclarations(elf);
  pepp::Architecture arch = pepp::Architecture::NO_ARCH;
  // determine arch from ELF.
  switch (elf.get_machine()) {
  case (((quint16)'p') << 8) | ((quint16)'9'): arch = pepp::Architecture::PEP9; break;
  case (((quint16)'p') << 8) | ((quint16)'x'): arch = pepp::Architecture::PEP10; break;
  default: throw std::logic_error("Unimplemented architecture");
  }

  reconfigure(arch, memmap, mmios);
}

// Using previous allocated Dense memory devices, allocate a "new" Dense memory with the specified parameters.
// Will attempt to find an exact match in the pool, and if not, will attempt to find the smallest memory that can fit
// the request. If no memory can fit the request, allocate a new Dense memory.
QSharedPointer<sim::memory::Dense<quint16>> allocate(QVector<QSharedPointer<sim::memory::Dense<quint16>>> &pool,
                                                     sim::api2::device::Descriptor desc, AddressSpan span) {
  // Attempt to find exact match in pool.
  for (int index = 0; index < pool.size(); index++) {
    if (sim::api2::memory::size_inclusive(pool[index]->span()) == sim::api2::memory::size_inclusive(span)) {
      auto ret = pool.takeAt(index);
      ret->setDevice(desc);
      return ret;
    }
  }

  // TODO: consider using capacity rather than size, because additional memory may be reserved due to previous resizing.
  // Compute the difference in sizes between the request and actual sizes. Positive and close to 0 is best.
  QVector<std::size_t> size_diff(pool.size(), 0);
  std::transform(pool.cbegin(), pool.cend(), size_diff.begin(), [span](const auto &mem) {
    return sim::api2::memory::size_inclusive(mem->span()) - sim::api2::memory::size_inclusive(span);
  });
  // Find the element smallest element that can contain the request without expanding.
  std::size_t min_diff = std::numeric_limits<std::size_t>::max(), index = std::numeric_limits<std::size_t>::max();
  for (int i = 0; i < size_diff.size(); i++) {
    if (size_diff[i] >= 0 && size_diff[i] < min_diff) {
      min_diff = size_diff[i];
      index = i;
    }
  }

  // No element large enough to hold, so allocate a new one.
  if (index >= pool.size()) return QSharedPointer<sim::memory::Dense<quint16>>::create(desc, span);
  // Otherwise re-use the selected memory, resizing the span.
  auto ret = pool.takeAt(index);
  ret->setDevice(desc);
  ret->setSpan(span);
  return ret;
}

void targets::isa::System::reconfigure(pepp::Architecture arch, QList<obj::MemoryRegion> regions,
                                       QList<obj::AddressedIO> mmios) {
  using enum pepp::Architecture;
  // Take underlying memory and reuse it for new devices.
  using std::swap;
  decltype(_rawMemory) rawPool = {};
  swap(rawPool, _rawMemory);

  // Removed cached MMIO & RO devices, which should be cheap-ish except for IDE controller.
  _mmi.clear(), _mmo.clear(), _ide.clear(), _devices.clear(), _readonly.clear();
  _regions.clear();
  _nextID = _bus->deviceID() + 1;

  // Reset bus and path management.
  _paths->clear();
  _bus->removeAllTargets();
  _paths->add(0, _bus->deviceID());

  // Construct Dense memory, ignoring W bit, since we have no mechanism for it.
  for (const auto &reg : regions) {
    auto span = AddressSpan(0, static_cast<quint16>(reg.maxOffset - reg.minOffset));
    auto desc = desc_dense(nextID());
    auto mem = allocate(rawPool, desc, span);
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
    } else if (mmio.type == obj::IO::Type::kOutput) {
      auto desc = desc_mmo(nextID(), mmio.name);
      addDevice(desc);
      auto mem = QSharedPointer<sim::memory::Output<quint16>>::create(desc, span);
      _bus->pushFrontTarget(AddressSpan(mmio.minOffset, mmio.maxOffset), &*mem);
      _mmo[mmio.name] = mem;
    } else if (mmio.type == obj::IO::Type::kIDE) {
      auto desc = desc_ide(nextID(), mmio.name);
      addDevice(desc);
      auto mem = QSharedPointer<sim::memory::IDEController>::create(desc, 0, _nextIDGenerator);
      mem->setTarget(&*_bus, nullptr);
      _bus->pushFrontTarget(AddressSpan(mmio.minOffset, mmio.maxOffset), &*mem);
      _ide[mmio.name] = mem;
    } else {
      throw std::logic_error("Unreachable");
    }
  }
  // Use braces to limit scope of variables in switch.
  switch (arch) {
  case PEP9: {
    targets::pep9::isa::CPU *cpu = dynamic_cast<targets::pep9::isa::CPU *>(_cpu.data());
    addDevice(cpu->device());
    addDevice(cpu->csrs()->device());
    addDevice(cpu->regs()->device());
    // Add pwrOff if it does not already exist.
    if (_mmo.find("pwrOff") == _mmo.end()) {
      auto desc = desc_mmo(nextID(), "pwrOff");
      addDevice(desc);
      _mmo["pwrOff"] = QSharedPointer<sim::memory::Output<quint16>>::create(desc, AddressSpan(0, 0));
    }
    cpu->setPwrOff(_mmo["pwrOff"].data());
    cpu->setTarget(&*_bus, nullptr);
    break;
  }
  case PEP10: {
    targets::pep10::isa::CPU *cpu = dynamic_cast<targets::pep10::isa::CPU *>(_cpu.data());
    addDevice(cpu->device());
    addDevice(cpu->csrs()->device());
    addDevice(cpu->regs()->device());
    cpu->setTarget(&*_bus, nullptr);
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }

  // re-propogate trace buffer to any new devices.
  auto buf = _bus->buffer();
  _bus->setBuffer(buf);
}

void targets::isa::System::appendReloadEntries(QSharedPointer<sim::api2::memory::Target<quint16>> mem,
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

QSharedPointer<targets::isa::System> targets::isa::systemFromElf(const ELFIO::elfio &elf, bool loadUserImmediate) {
  using size_type = bits::span<const quint8>::size_type;
  auto segs = obj::getLoadableSegments(elf);
  auto memmap = obj::mergeSegmentRegions(segs);
  auto mmios = obj::getMMIODeclarations(elf);
  pepp::Architecture arch = pepp::Architecture::NO_ARCH;
  // determine arch from ELF.
  switch (elf.get_machine()) {
  case (((quint16)'p') << 8) | ((quint16)'9'): arch = pepp::Architecture::PEP9; break;
  case (((quint16)'p') << 8) | ((quint16)'x'): arch = pepp::Architecture::PEP10; break;
  default: throw std::logic_error("Unimplemented architecture");
  }

  auto ret = QSharedPointer<targets::isa::System>::create(arch, memmap, mmios);

  // Either immediately load user program into memory, or buffer behind correct
  // port.
  if (loadUserImmediate) {
    quint16 address = 0;
    auto bus = ret->bus();
    auto seg = elf.segments[0];
    auto ptr = reinterpret_cast<const quint8 *>(seg->get_data());
    const auto ret = bus->write(address, {ptr, static_cast<size_type>(seg->get_file_size())}, gs);
  } else {
    auto seg = elf.segments[0];
    auto ptr = reinterpret_cast<const quint8 *>(seg->get_data());
    auto mmi = ret->input("charIn");
    Q_ASSERT(mmi != nullptr);
    auto endpoint = mmi->endpoint();
    auto bytesAsHex = obj::segmentAsAsciiHex(seg);
    for (auto byte : bytesAsHex) endpoint->append_value(byte);
  }
  return ret;
}

bool targets::isa::loadRegion(sim::api2::memory::Target<quint16> &mem, const obj::MemoryRegion &reg,
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

bool targets::isa::loadElfSegments(sim::api2::memory::Target<quint16> &mem, const ELFIO::elfio &elf) {
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
