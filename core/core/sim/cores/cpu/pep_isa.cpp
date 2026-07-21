#include "pep_isa.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

PepISA3CPU::PepISA3CPU(Configuration cfg, System *sys) : _config(cfg) {
  {
    Dense::Configuration cfg;
    cfg.basename = "regs";
    cfg.fill = 0;
    cfg.span = {0, 31 * sizeof(u16)};
    cfg.skip_serialize = true;
    _regbank = sys->make_device<Dense>(cfg);
  }
  {
    Dense::Configuration cfg;
    cfg.basename = "csrs";
    cfg.fill = 0;
    // N, Z, V, C
    cfg.span = {0, 3};
    cfg.skip_serialize = true;
    _csrs = sys->make_device<Dense>(cfg);
  }
}

void PepISA3CPU::initialize(System *sys) {
  auto dev = sys->find_relative(_config.target, _config.fullname);
  if (!dev) throw std::runtime_error("PepISA3CPU: could not find target device " + _config.target);
  _target = dev->capability<Target>();
  if (!_target) throw std::runtime_error("PepISA3CPU: device " + _config.target + " is not a memory target");
}

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
  _regbank->set_buffer(tb);
  _csrs->set_buffer(tb);
}

const Buffer *PepISA3CPU::buffer() const { return _tb; }

bool PepISA3CPU::can_generate_traces() const { return true; }

void PepISA3CPU::trace(bool enabled) {
  if (_tb) {
    _tb->trace(id(), enabled);
    _regbank->trace(enabled);
    _csrs->trace(enabled);
  }
}

bool PepISA3CPU::traced() const { return _tb ? _tb->traced(id()) : false; }
