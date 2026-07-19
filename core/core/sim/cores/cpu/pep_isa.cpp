#include "pep_isa.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

PepISA3CPU::PepISA3CPU(Configuration cfg, System *sys) : _config(cfg) {}

void PepISA3CPU::initialize(System *) {}

const Device::Configuration &PepISA3CPU::config() const { return _config; }

const PepISA3CPU::Configuration &PepISA3CPU::casted_config() const { return _config; }

const Device::ID PepISA3CPU::id() const { return _config.id; }

Device::Type PepISA3CPU::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::ClockSink | T::Traceable | T::MemoryInitiator;
}

std::unique_ptr<DeviceSerializer> PepISA3CPU::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> PepISA3CPU::make_serializer() { return nullptr; }

void PepISA3CPU::clock_tick(PulseSchedule::PulseIndex idx, u64 tick) { (void)idx, (void)tick; }

void PepISA3CPU::set_clock_source(const ClockSource *src) { _clk = src; }

const ClockSource *PepISA3CPU::clock_source() const { return _clk; }

void PepISA3CPU::set_buffer(Buffer *tb) {
  _tb = tb;
  // TODO: set tb on child devices
}

const Buffer *PepISA3CPU::buffer() const { return _tb; }

bool PepISA3CPU::can_generate_traces() const { return true; }

void PepISA3CPU::trace(bool enabled) {
  if (_tb) _tb->trace(id(), enabled);
  // TODO: also set trace on children devices (regbank, csrs)
}

bool PepISA3CPU::traced() const { return _tb ? _tb->traced(id()) : false; }
