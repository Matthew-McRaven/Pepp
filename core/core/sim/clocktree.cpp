#include "./clocktree.hpp"
#include "system.hpp"

pepp::ScaledClock::ScaledClock(Configuration config) : Device(), ClockSource(), _config(config) {
  if (*_config.jitter_scale != *_config.jitter_scale) _config.jitter_scale = *_config.period_scale;
}

void pepp::ScaledClock::initialize(System *sys) {
  auto dev = sys->find_relative(*_config.parent_name, *_config.fullname);
  if (!dev) throw std::runtime_error("ScaledClockNode: could not find parent clock " + *_config.parent_name);
  auto clk = dev->capability<ClockSource>();
  if (!clk) throw std::runtime_error("ScaledClockNode: device " + *_config.parent_name + " is not a clock source");
  _parent = clk;
}

PulseSchedule pepp::ScaledClock::schedule() const {
  if (!_parent) throw std::runtime_error("ScaledClockNode: parent clock not set");
  const auto par = _parent->schedule();
  return PulseSchedule{.period = static_cast<u64>(par.period * *_config.period_scale),
                       .jitter = static_cast<u64>(par.jitter * *_config.jitter_scale),
                       .seed = par.seed};
}

pepp::MuxClock::MuxClock(Configuration config) : Device(), ClockSource(), _index(0), _config(config) {}

void pepp::MuxClock::initialize(System *sys) {
  for (const auto &name : *_config.names) {
    auto dev = sys->find_relative(name, *_config.fullname);
    if (!dev) throw std::runtime_error("MuxClockNode: could not find clock " + name);
    auto clk = dev->capability<ClockSource>();
    if (!clk) throw std::runtime_error("MuxClockNode: device " + name + " is not a clock source");
    _choices.push_back(clk);
  }
  select_clock(*_config.selected);
}

void pepp::MuxClock::select_clock(u16 index) {
  if (index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
  else if (index == _index) return; // No change
  _index = index;
}

PulseSchedule pepp::MuxClock::schedule() const {
  if (_index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
  return _choices[_index]->schedule();
}

const ClockSource *pepp::MuxClock::selected_clock() const {
  if (_index > _choices.size()) return nullptr;
  else return _choices[_index];
}
