#include "sim_clocktree.hpp"

void pepp::ClockGovernor::map_device_clock(ID device, ID clock) {
  auto ev = _loop.allocator.alloc<ClockReceipt>(device);
  ev->clock = clock;
  ev->base.recurs = true;
  if (_device_to_clock.size() <= device.value) {
    _device_to_clock.resize(device.value + 1, Device::ID{0});
    _device_to_event.resize(device.value + 1, nullptr);
  }
  _device_to_clock[device.value] = clock;
  _device_to_event[device.value] = ev;
  _events_to_schedule.emplace_back(ev);
}

void pepp::ClockGovernor::request_clock(ID device) {
  auto ev = _device_to_event[device.value];
  const auto clock_id = _device_to_clock[device.value];
  const auto &clock = _clocks[clock_id];
  // const auto next_time = clock->;
  // const auto delay = _clocks[_device_to_clock[device.value]].;
  const u64 delay = 0;
  _loop.scheduler.schedule(ev->base.event_id, delay);
}

void pepp::ClockGovernor::handle_event(const Event *ev) {
  if (ev->type != Event::Type::UpdateClockSchedule) return;
  const auto current_time = _loop.scheduler.current_tick();
  for (const auto &ev : _events_to_schedule) {
    const auto &[clock, old_schedule] = _clocks[ev->clock];
    const auto new_schedule = clock->schedule();
    if (old_schedule == new_schedule && _loop.scheduler.scheduled(ev->base.event_id)) continue;
    const auto next_tick = new_schedule.next_clock_tick(current_time);
    const auto delay = next_tick - current_time;
    _loop.scheduler.schedule(ev->base.event_id, delay);
    _clocks[ev->clock] = std::make_pair(clock, new_schedule);
  }
}
constexpr u64 pepp::PulseSchedule::next_clock_tick(u64 tick) const noexcept {
  // get pulse index for current tick, increment it
  const auto idx = ++index_of(tick);
  return edge_time(idx);
}
