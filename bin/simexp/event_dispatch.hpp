#pragma once
#include <vector>
#include "./events.hpp"
#include "core/integers.h"

class EventDispatcher {
public:
  struct Entry {
    u8 source;
    Event::Type type;
    bool operator==(Entry const &other) const = default;
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
  void register_device(Handler *handler);
  u16 hash(u8 source, Event::Type type) const;
  void register_handler(u8 source, Event::Type ev, u8 handler);
  u8 handler_for(Entry entry) const;
  u8 handler_for(u8 source, Event::Type ev) const;

  void handle_event(const Event *ev) const;
  template <typename DerivedFilter, typename... Args>
  DerivedFilter *install_filter(EventDispatcher::Entry target, Args &&...args);

private:
  // The handler function for a specific device ID.
  std::vector<Handler *> _devices;
  // A vector sized to event_type_count() * # of devices.
  // The handle for (device, ev) is at [device*event_type_count() + (u8)ev).
  // Since 0 is a reserved device ID, we use 0 as a sentinel value to indicate that no handler is registered for a
  // device/event pair. When handlers gets too large and starts having poor memory performance, switch to
  // ankerl::unordered_dense::map<EventEntry, u8, EventEntry::Hash>.
  std::vector<u8> _handlers;
};
template <typename Derived>
EventDispatcher::Filter<Derived>::Filter(EventDispatcher &disp, u8 previous) : _disp(disp), _previous(previous) {}

template <typename Derived> void EventDispatcher::Filter<Derived>::handle_event(const Event *ev) {
  if (static_cast<Derived *>(this)->filter(ev))
    if (auto hnd = _disp._devices[_previous]; hnd) hnd->handle_event(ev);
}

template <typename DerivedFilter, typename... Args>
inline DerivedFilter *EventDispatcher::install_filter(Entry target, Args &&...args) {
  static_assert(std::derived_from<DerivedFilter, EventDispatcher::Filter<DerivedFilter>>,
                "Filter must derive from EventLoop::EventFilter");
  auto handler = this->handler_for(target);
  auto ret = new DerivedFilter(*this, handler, std::forward<Args>(args)...);
  ret->_id = 3;
  register_device(ret);
  register_handler(target.source, target.type, ret->id());
  return ret;
}
