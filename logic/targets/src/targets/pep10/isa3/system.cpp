#include "./system.hpp"
#include "obj/memmap.hpp"
#include "obj/mmio.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/readonly.hpp"
#include "sim/device/simple_bus.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"

using AddressSpan = sim::api::memory::Target<quint16>::AddressSpan;
sim::api::device::Descriptor desc_cpu(sim::api::device::ID id) {
  return {.id = id, .baseName = "cpu", .fullName = "/cpu"};
}
sim::api::device::Descriptor desc_bus(sim::api::device::ID id) {
  return {.id = id, .baseName = "bus", .fullName = "/bus"};
}
sim::api::device::Descriptor desc_dense(sim::api::device::ID id) {
  return {.id = id,
          .baseName = u"dense%1"_qs.arg(id),
          .fullName = u"/bus/dense%1"_qs.arg(id)};
}
sim::api::device::Descriptor desc_mmi(sim::api::device::ID id, QString name) {
  return {.id = id,
          .baseName = u"mmi-%1"_qs.arg(name),
          .fullName = u"/bus/mmi-%1"_qs.arg(name)};
}
sim::api::device::Descriptor desc_mmo(sim::api::device::ID id, QString name) {
  return {.id = id,
          .baseName = u"mmo-%1"_qs.arg(name),
          .fullName = u"/bus/mmo-%1"_qs.arg(name)};
}

sim::api::memory::Operation rw = {.speculative = false,
                                  .kind =
                                      sim::api::memory::Operation::Kind::data,
                                  .effectful = true};

targets::pep10::isa::System::System(QList<obj::MemoryRegion> regions,
                                    QList<obj::AddressedIO> mmios)
    : _cpu(QSharedPointer<CPU>::create(desc_cpu(nextID()), _nextIDGenerator)),
      _bus(QSharedPointer<sim::memory::SimpleBus<quint16>>::create(
          desc_bus(nextID()),
          AddressSpan{.minOffset = 0, .maxOffset = 0xFFFF})) {
  // Construct Dense memory and ignore W bit, since we have no mechanism for it.
  for (const auto &reg : regions) {
    auto span = AddressSpan{
        .minOffset = 0,
        .maxOffset = static_cast<quint16>(reg.maxOffset - reg.minOffset)};
    auto mem = QSharedPointer<sim::memory::Dense<quint16>>::create(
        desc_dense(nextID()), span);
    _rawMemory.push_back(mem);
    sim::api::memory::Target<quint16> *target = &*mem;
    if (!reg.w) {
      auto ro = QSharedPointer<sim::memory::ReadOnly<quint16>>::create(false);
      _readonly.push_back(ro);
      ro->setTarget(target);
      target = &*ro;
    }
    _bus->pushFrontTarget(
        {.minOffset = reg.minOffset, .maxOffset = reg.maxOffset}, target);

    // Perform load!
    quint16 base = 0;
    for (const auto seg : reg.segs) {
      auto fileData = seg->get_data();
      auto size = seg->get_file_size();
      if (fileData == nullptr)
        continue;
      mem->write(base, reinterpret_cast<const quint8 *>(fileData), size, rw);
      base += size;
    }
  }

  // Create MMIO, do not perform buffering
  for (const auto &mmio : mmios) {
    auto span = AddressSpan{
        .minOffset = 0,
        .maxOffset = static_cast<quint16>(mmio.maxOffset - mmio.minOffset)};
    if (mmio.direction == obj::IO::Direction::kInput) {
      auto mem = QSharedPointer<sim::memory::Input<quint16>>::create(
          desc_mmi(nextID(), mmio.name), span);
      _bus->pushFrontTarget(
          AddressSpan{.minOffset = mmio.minOffset, .maxOffset = mmio.maxOffset},
          &*mem);
      _mmi[mmio.name] = mem;
    } else {
      auto mem = QSharedPointer<sim::memory::Output<quint16>>::create(
          desc_mmi(nextID(), mmio.name), span);
      _bus->pushFrontTarget(
          AddressSpan{.minOffset = mmio.minOffset, .maxOffset = mmio.maxOffset},
          &*mem);
      _mmo[mmio.name] = mem;
    }
  }
  _cpu->setTarget(&*_bus);
}

std::pair<sim::api::tick::Type, sim::api::tick::Result>
targets::pep10::isa::System::tick(sim::api::Scheduler::Mode mode) {
  auto res = _cpu->tick(_tick);
  return {res.error == sim::api::tick::Error::Success ? ++_tick : _tick, res};
}

sim::api::tick::Type targets::pep10::isa::System::currentTick() const {
  return _tick;
}

sim::api::device::ID targets::pep10::isa::System::nextID() { return _nextID++; }

sim::api::device::IDGenerator targets::pep10::isa::System::nextIDGenerator() {
  return _nextIDGenerator;
}

void targets::pep10::isa::System::setTraceBuffer(
    sim::api::trace::Buffer *buffer) {
  throw std::logic_error("Unimplemented");
}

void targets::pep10::isa::System::setBootFlagAddress(quint16 addr) {
  _bootFlg = addr;
}

void targets::pep10::isa::System::setBootFlags(bool enableLoader,
                                               bool enableDispatcher) {
  quint16 value = (enableLoader ? 1 << 0 : 0) | (enableDispatcher ? 1 << 1 : 0);
  if (_bootFlg) {
    auto ret =
        _bus->write(*_bootFlg, reinterpret_cast<quint8 *>(&value), 2, rw);
    Q_ASSERT(ret.completed);
  }
}

std::optional<quint16> targets::pep10::isa::System::getBootFlagAddress() {
  return _bootFlg;
}

quint16 targets::pep10::isa::System::getBootFlags() const {
  quint8 buf[2];
  bits::memclr(buf, 2);
  if (_bootFlg) {
    auto ret = _bus->read(*_bootFlg, buf, 2, rw);
    Q_ASSERT(ret.completed);
  }
  return bits::memcpy_endian<quint16>(buf, bits::Order::BigEndian, 2);
}

void targets::pep10::isa::System::init() {
  quint8 buf[2];
  // Initalize PC to dispatcher
  _bus->read(static_cast<quint16>(::isa::Pep10::MemoryVectors::Dispatcher), buf,
             2, rw);
  writeRegister(cpu()->regs(), ::isa::Pep10::Register::PC,
                bits::memcpy_endian<quint16>(buf, bits::Order::BigEndian, 2),
                rw);
  // Initalize SP to system stack pointer
  _bus->read(static_cast<quint16>(::isa::Pep10::MemoryVectors::SystemStackPtr),
             buf, 2, rw);
  writeRegister(cpu()->regs(), ::isa::Pep10::Register::SP,
                bits::memcpy_endian<quint16>(buf, bits::Order::BigEndian, 2),
                rw);
}

targets::pep10::isa::CPU *targets::pep10::isa::System::cpu() { return &*_cpu; }

sim::memory::SimpleBus<quint16> *targets::pep10::isa::System::bus() {
  return &*_bus;
}

QStringList targets::pep10::isa::System::inputs() const { return _mmi.keys(); }

sim::memory::Input<quint16> *targets::pep10::isa::System::input(QString name) {
  if (auto find = _mmi.find(name); find != _mmi.end())
    return &**find;
  return nullptr;
}

QStringList targets::pep10::isa::System::outputs() const { return _mmo.keys(); }

sim::memory::Output<quint16> *
targets::pep10::isa::System::output(QString name) {
  if (auto find = _mmo.find(name); find != _mmo.end())
    return &**find;
  return nullptr;
}

QSharedPointer<targets::pep10::isa::System>
targets::pep10::isa::systemFromElf(const ELFIO::elfio &elf,
                                   bool loadUserImmediate) {
  auto segs = obj::getLoadableSegments(elf);
  auto memmap = obj::mergeSegmentRegions(segs);
  auto mmios = obj::getMMIODeclarations(elf);
  auto buffers = obj::getMMIBuffers(elf);

  auto ret = QSharedPointer<targets::pep10::isa::System>::create(memmap, mmios);

  // Either immediately load user program into memory, or buffer behind correct
  // port.
  if (loadUserImmediate) {
    quint16 address = 0;
    auto bus = ret->bus();
    for (auto buffer : buffers) {
      auto ptr = reinterpret_cast<const quint8 *>(buffer.seg->get_data());
      if (ptr == nullptr)
        continue;
      const auto ret =
          bus->write(address, ptr, buffer.seg->get_memory_size(), rw);
      Q_ASSERT(ret.completed);
      Q_ASSERT(ret.error == sim::api::memory::Error::Success);
      address += buffer.seg->get_memory_size();
    }
  } else {
    for (auto buffer : buffers) {
      auto mmi = ret->input(buffer.portName);
      auto endpoint = mmi->endpoint();
      auto seg = buffer.seg;
      Q_ASSERT(mmi != nullptr);
      for (int i = 0; i < seg->get_memory_size(); i++) {
        auto byte = i < seg->get_file_size() ? seg->get_data()[i] : 0;
        endpoint->append_value(byte);
      }
    }
  }

  if (auto bootFlg = obj::getBootFlagsAddress(elf); bootFlg)
    ret->setBootFlagAddress(*bootFlg);

  return ret;
}
