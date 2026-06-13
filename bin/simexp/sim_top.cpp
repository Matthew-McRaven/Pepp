#include "sim_top.hpp"

void Simulator::init_clocks() {
  using Key = EventDispatcher::DispatchKey;
  const auto self_id = Device::ID(0);
  const auto ev = allocator().alloc<UpdateClockScheduleEvent>(self_id);
  // Ensure clock manager is register
  dispatcher().add_handler(clocks.id(), &clocks);
  dispatcher().map_handler(Key{self_id, Event::Type::UpdateClockSchedule}, clocks.id());
  scheduler().schedule(ev->base.event_id, 0);
}
