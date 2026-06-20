#include "traced_pep_ma2_system.hpp"
#include "core/math/bitmanip/enums.hpp"
using namespace Qt::StringLiterals;
using AddressSpan = sim::api2::memory::AddressSpan<u16>;
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
} // namespace

targets::ma::System::System(pepp::Architecture_Enum arch, pepp::Features feats)
    : _arch(arch),
      _bus(QSharedPointer<sim::memory::SimpleBus<u16>>::create(desc_bus(nextID()), AddressSpan(0, 0xFFFF))),
      _paths(QSharedPointer<sim::api2::Paths>::create()) {
  _rawMemory = QSharedPointer<sim::memory::Dense<u16>>::create(desc_dense(nextID()), AddressSpan(0, 0xFFFF));
  _paths->clear();
  _paths->add(0, _bus->deviceID());
  _bus->pushFrontTarget(AddressSpan(0, 0xFFFF), _rawMemory.get());
  using namespace bits;
  switch (arch) {
  case pepp::Architecture_Enum::PEP8: _feats = pepp::Features::OneByte; _cpu = nullptr;
  case pepp::Architecture_Enum::PEP9: [[fallthrough]];
  case pepp::Architecture_Enum::PEP10:
    if (any(pepp::Features::TwoByte & feats)) {
      _feats = pepp::Features::TwoByte;
      _cpu = QSharedPointer<targets::pep9::mc2::CPUWordBus>::create(desc_cpu(nextID()), _nextIDGenerator);
    } else {
      _feats = pepp::Features::OneByte;
      _cpu = QSharedPointer<targets::pep9::mc2::CPUByteBus>::create(desc_cpu(nextID()), _nextIDGenerator);
    }
  default: _feats = pepp::Features::None; break;
  }
  if (_cpu) _cpu->setTarget(&*_bus, nullptr);
}

std::pair<sim::api2::tick::Type, sim::api2::tick::Result> targets::ma::System::tick(sim::api2::Scheduler::Mode mode) {
  auto tb = _bus->buffer();
  if (tb) tb->emitFrameStart();
  auto res = _cpu->clock(_tick);
  if (tb) tb->updateFrameHeader();
  return {++_tick, res};
}

sim::api2::tick::Type targets::ma::System::currentTick() const { return _tick; }

sim::api2::device::ID targets::ma::System::nextID() { return _nextID++; }

sim::api2::device::IDGenerator targets::ma::System::nextIDGenerator() { return _nextIDGenerator; }

void targets::ma::System::addDevice(sim::api2::device::Descriptor desc) { _devices[desc.id] = desc; }

sim::api2::device::Descriptor *targets::ma::System::descriptor(sim::api2::device::ID id) {
  if (auto it = _devices.find(id); it == _devices.cend()) return nullptr;
  else return &it.value();
}

void targets::ma::System::setBuffer(sim::api2::trace::Buffer *) {
  static const char *const e = "Unimplemented";
  qCritical(e);
  throw std::logic_error(e);
}

QSharedPointer<const sim::api2::Paths> targets::ma::System::pathManager() const { return _paths; }

void targets::ma::System::init() {
  cpu()->init();
  _tick = 0;
}

pepp::Architecture_Enum targets::ma::System::architecture() const { return _arch; }

targets::pep9::mc2::BaseCPU *targets::ma::System::cpu() { return _cpu.get(); }

sim::memory::SimpleBus<u16> *targets::ma::System::bus() { return _bus.get(); }
