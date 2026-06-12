#pragma once
#include <vector>
#include "./events.hpp"
#include "core/integers.h"

class EventDispatcher {
public:
  struct DispatchKey {
    u8 source;
    Event::Type type;
    bool operator==(DispatchKey const &other) const = default;
  };
  struct Handler {
    virtual ~Handler() = default;
    virtual void handle_event(const Event *ev) = 0;
    virtual u8 id() const = 0;
  };
  template <typename Derived> struct Filter : public EventDispatcher::Handler {
    virtual ~Filter() = default;
    Filter(EventDispatcher &loop, u8 previous);
    void handle_event(const Event *ev) override;

  private:
    EventDispatcher &_disp;
    u8 _previous;
  };
  void add_handler(Handler *handler);
  void map_handler(u8 source, Event::Type ev, u8 handler);
  void map_handler(DispatchKey, u8 handler);
  u8 handler_for(DispatchKey DispatchKey) const;
  u8 handler_for(u8 source, Event::Type ev) const;

  void dispatch(const Event *ev) const;
  template <typename DerivedFilter, typename... Args>
  DerivedFilter *install_filter(u8 filter_id, DispatchKey DispatchKey, Args &&...args);

private:
  // The handler function for a specific device ID.
  std::vector<Handler *> _handlers;
  // A vector sized to event_type_count() * # of devices.
  // The handle for (device, ev) is at [device*event_type_count() + (u8)ev).
  // Since 0 is a reserved device ID, we use 0 as a sentinel value to indicate that no handler is registered for a
  // device/event pair. When handlers gets too large and starts having poor memory performance, switch to
  // ankerl::unordered_dense::map<EventEntry, u8, EventEntry::Hash>.
  constexpr u16 hash(u8 source, Event::Type type) const noexcept {
    return static_cast<u16>(source) * event_type_count() + static_cast<u8>(type);
  }
  constexpr u16 hash(DispatchKey DispatchKey) const noexcept { return hash(DispatchKey.source, DispatchKey.type); }
  std::vector<u8> _dispatch_table;
};
template <typename Derived>
EventDispatcher::Filter<Derived>::Filter(EventDispatcher &disp, u8 previous) : _disp(disp), _previous(previous) {}

template <typename Derived> void EventDispatcher::Filter<Derived>::handle_event(const Event *ev) {
  if (static_cast<Derived *>(this)->filter(ev))
    if (auto hnd = _disp._handlers[_previous]; hnd) hnd->handle_event(ev);
}

template <typename DerivedFilter, typename... Args>
inline DerivedFilter *EventDispatcher::install_filter(u8 filter_id, DispatchKey DispatchKey, Args &&...args) {
  static_assert(std::derived_from<DerivedFilter, EventDispatcher::Filter<DerivedFilter>>,
                "Filter must derive from EventLoop::EventFilter");
  auto handler = this->handler_for(DispatchKey);
  auto ret = new DerivedFilter(*this, handler, std::forward<Args>(args)...);
  ret->_id = filter_id;
  add_handler(ret);
  map_handler(DispatchKey, ret->id());
  return ret;
}
