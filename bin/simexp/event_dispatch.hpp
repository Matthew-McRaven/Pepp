#pragma once
#include <vector>
#include "./events.hpp"
#include "./sim_device.hpp"
#include "core/integers.h"

class EventDispatcher {
public:
  struct DispatchKey {
    Device::ID source;
    Event::Type type;
    bool operator==(DispatchKey const &other) const = default;
  };
  struct Handler {
    virtual ~Handler() = default;
    virtual void handle_event(const Event *ev) = 0;
    virtual Device::ID id() const = 0;
  };
  template <typename ConcreteFilter> struct Filter : public EventDispatcher::Handler {
    virtual ~Filter() = default;
    Filter(EventDispatcher &loop, Device::ID previous);
    void handle_event(const Event *ev) override;

  private:
    EventDispatcher &_disp;
    Device::ID _previous;
  };
  void add_handler(Device::ID name, Handler *handler);
  void map_handler(Device::ID source, Event::Type ev, Device::ID handler);
  void map_handler(DispatchKey, Device::ID handler);
  Device::ID handler_for(DispatchKey DispatchKey) const;
  Device::ID handler_for(Device::ID source, Event::Type ev) const;

  void dispatch(const Event *ev) const;
  template <typename ConcreteFilter, typename... Args>
  ConcreteFilter *install_filter(Device::ID filter_id, DispatchKey DispatchKey, Args &&...args);

private:
  // The handler function for a specific device ID.
  std::vector<Handler *> _handlers;
  // A vector sized to event_type_count() * # of devices.
  // The handle for (device, ev) is at [device*event_type_count() + (u8)ev).
  // Since 0 is a reserved device ID, we use 0 as a sentinel value to indicate that no handler is registered for a
  // device/event pair. When handlers gets too large and starts having poor memory performance, switch to
  // ankerl::unordered_dense::map<EventEntry, u8, EventEntry::Hash>.
  constexpr u16 hash(Device::ID source, Event::Type type) const noexcept {
    return static_cast<u16>(source.value) * event_type_count() + static_cast<u8>(type);
  }
  constexpr u16 hash(DispatchKey DispatchKey) const noexcept { return hash(DispatchKey.source, DispatchKey.type); }
  std::vector<Device::ID::underlying_type> _dispatch_table;
};
template <typename ConcreteFilter>
EventDispatcher::Filter<ConcreteFilter>::Filter(EventDispatcher &disp, Device::ID previous)
    : _disp(disp), _previous(previous) {}

template <typename ConcreteFilter> void EventDispatcher::Filter<ConcreteFilter>::handle_event(const Event *ev) {
  if (static_cast<ConcreteFilter *>(this)->filter(ev))
    if (auto hnd = _disp._handlers[_previous.value]; hnd) hnd->handle_event(ev);
}

template <typename ConcreteFilter, typename... Args>
inline ConcreteFilter *EventDispatcher::install_filter(Device::ID filter_id, DispatchKey DispatchKey, Args &&...args) {
  static_assert(std::derived_from<ConcreteFilter, EventDispatcher::Filter<ConcreteFilter>>,
                "Filter must derive from EventLoop::EventFilter");
  auto handler = this->handler_for(DispatchKey);
  auto ret = new ConcreteFilter(*this, handler, filter_id, std::forward<Args>(args)...);
  add_handler(filter_id, ret);
  map_handler(DispatchKey, filter_id);
  return ret;
}
