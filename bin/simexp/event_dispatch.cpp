#include "event_dispatch.hpp"
#include "fmt/format.h"

void EventDispatcher::add_handler(Handler *handler) {
  const auto id = handler->id();
  const auto size = id + 1;
  if (_handlers.size() < size) _handlers.resize(size, nullptr);
  _handlers[id] = handler;
}

void EventDispatcher::map_handler(DispatchKey route, u8 handler) {
  const auto size = route.source + 1;
  if (_handlers.size() < size * event_type_count()) _handlers.resize(size * event_type_count(), 0);
  _dispatch_table[hash(route)] = handler;
}

void EventDispatcher::map_handler(u8 source, Event::Type ev, u8 handler) {
  return map_handler(DispatchKey{source, ev}, handler);
}

u8 EventDispatcher::handler_for(DispatchKey DispatchKey) const {
  if (DispatchKey.source >= _handlers.size() / event_type_count()) return 0;
  else if (const auto handler = _dispatch_table[hash(DispatchKey)]; handler == 0 || handler >= _handlers.size())
    return 0;
  else return handler;
}

u8 EventDispatcher::handler_for(u8 source, Event::Type ev) const { return handler_for(DispatchKey{source, ev}); }

void EventDispatcher::dispatch(const Event *ev) const {
  Handler *hnd = nullptr;
  const auto target = _dispatch_table[hash(ev->source, ev->type)];
  if (target == 0 || (hnd = _handlers[target]) == nullptr) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("No handler registered for event type {} from source {}", static_cast<u8>(ev->type), ev->source));
  } else hnd->handle_event(ev);
}