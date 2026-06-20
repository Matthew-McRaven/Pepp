#include "event_dispatch.hpp"
#include "fmt/format.h"

void EventDispatcher::add_handler(Device::ID id, Handler *handler) {
  const auto size = id.value + 1;
  if (_handlers.size() < size) _handlers.resize(size, nullptr);
  _handlers[id.value] = handler;
}

void EventDispatcher::map_handler(DispatchKey route, Device::ID handler) {
  const auto size = route.source.value + 1;
  if (_dispatch_table.size() < size * event_type_count()) _dispatch_table.resize(size * event_type_count(), 0);
  const auto hashed = hash(route);
  _dispatch_table[hashed] = handler.value;
}

void EventDispatcher::map_handler(Device::ID source, Event::Type ev, Device::ID handler) {
  return map_handler(DispatchKey{source, ev}, handler);
}

Device::ID EventDispatcher::handler_for(DispatchKey DispatchKey) const {
  if (DispatchKey.source.value >= _dispatch_table.size() / event_type_count()) return Device::ID{0};
  else if (const auto handler = _dispatch_table[hash(DispatchKey)]; handler == 0 || handler >= _dispatch_table.size())
    return Device::ID(0);
  else return Device::ID{handler};
}

Device::ID EventDispatcher::handler_for(Device::ID source, Event::Type ev) const {
  return handler_for(DispatchKey{source, ev});
}

void EventDispatcher::dispatch(const Event *ev) const {
  Handler *hnd = nullptr;
  const auto target = _dispatch_table[hash(ev->source, ev->type)];
  if (target == 0 || (hnd = _handlers[target]) == nullptr) [[unlikely]] {
    throw std::runtime_error(fmt::format("No handler registered for event type {} from source {}",
                                         static_cast<u8>(ev->type), ev->source.value));
  } else hnd->handle_event(ev);
}