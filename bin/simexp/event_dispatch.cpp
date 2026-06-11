#include "event_dispatch.hpp"
#include "fmt/format.h"

void EventDispatcher::register_device(Handler *handler) {
  const auto id = handler->id();
  const auto size = id + 1;
  if (_devices.size() < size) _devices.resize(size, nullptr);
  _devices[id] = handler;
}

u16 EventDispatcher::hash(u8 source, Event::Type type) const {
  return static_cast<u16>(source) * event_type_count() + static_cast<u8>(type);
}
void EventDispatcher::register_handler(u8 source, Event::Type ev, u8 handler) {
  const auto size = source + 1;
  if (_handlers.size() < size * event_type_count()) _handlers.resize(size * event_type_count(), 0);
  _handlers[hash(source, ev)] = handler;
}

u8 EventDispatcher::handler_for(Entry entry) const {
  if (entry.source >= _handlers.size() / event_type_count()) return 0;
  else if (const auto target = _handlers[hash(entry.source, entry.type)]; target == 0 || target >= _devices.size())
    return 0;
  else return target;
}

u8 EventDispatcher::handler_for(u8 source, Event::Type ev) const { return handler_for(Entry{source, ev}); }

void EventDispatcher::handle_event(const Event *ev) const {
  Handler *hnd = nullptr;
  const auto target = _handlers[hash(ev->source, ev->type)];
  if (target == 0 || (hnd = _devices[target]) == nullptr) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("No handler registered for event type {} from source {}", static_cast<u8>(ev->type), ev->source));
  } else hnd->handle_event(ev);
}