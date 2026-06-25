#pragma once
#include <map>
#include <memory>
#include <string>
#include "./event_loop.hpp"
#include "fmt/format.h"
#include "sim_clocktree.hpp"
#include "sim_device.hpp"

class Simulator : public Device {
  inline static const Descriptor _desc{.id = Device::ID{0}, .basename = "/", .fullname = "/"};
  inline static const Descriptor _clocks{
      .id = Device::ID{1}, .basename = "clocktree", .fullname = _desc.fullname + "clocktree"};

public:
  pepp::ClockGovernor clocks;
  Simulator() : Device(_desc), clocks(_clocks, _loop) {}
  ~Simulator() override = default;
  EventAllocator &allocator() { return _loop.allocator; }
  EventScheduler &scheduler() { return _loop.scheduler; }
  EventDispatcher &dispatcher() { return _loop.dispatcher; }
  EventLoop &loop() { return _loop; }

  EventLoop::Status run(u64 max_ticks) { return _loop.run(max_ticks); }
  EventLoop::Status run(std::function<bool()> pause) { return _loop.run(pause); }
  template <typename StopCondition> EventLoop::Status run(StopCondition &&stop);
  // If device has a member (EventLoop* loop), it will be assigned automatically.
  // Make a device that is a direct child of the this root device.
  template <typename ConcreteDevice, typename... Args>
  ConcreteDevice *make_device(std::string_view self_name, Args &&...args);
  // Make a device that is a child of a given device
  template <typename ConcreteDevice, typename... Args>
  ConcreteDevice *make_device(Device::ID parent, std::string_view self_name, Args &&...args);
  template <typename ConcreteDevice, typename... Args>
  ConcreteDevice *make_device(Device *parent, std::string_view self_name, Args &&...args);
  template <typename ConcreteClock, typename... Args>
  ConcreteClock *make_clock(std::string_view self_name, Args &&...args);

  template <typename ConcreteFilter, typename... Args>
  ConcreteFilter *make_filter(EventDispatcher::DispatchKey dispatch_key, Args &&...args);

  // Schedule the initial UpdateClockSchedule event.
  void init_clocks();

private:
  EventLoop _loop;
  Device::ID::underlying_type _next_id = 2;
  IDGenerator _next_id_gen = [this]() { return Device::ID{_next_id++}; };
  // A wrapper device for filters so that they can be querried in the device list and be destroyed correctly.
  // The actual EventHandler used by the dispatcher will be the stored filter member.
  // Both the filter member and the wrapper device have the same device ID.
  template <typename ConcreteFilter> struct FilterWrapper : public Device {
    FilterWrapper(Descriptor desc, EventDispatcher::Filter<ConcreteFilter> *ptr) : Device(desc), filter(ptr) {}
    std::unique_ptr<EventDispatcher::Filter<ConcreteFilter>> filter;
  };

  std::map<Device::ID, std::unique_ptr<Device>> _id_to_device;
};

template <typename StopCondition> EventLoop::Status Simulator::run(StopCondition &&stop) { return _loop.run(stop); }

template <typename ConcreteDevice, typename... Args>
ConcreteDevice *Simulator::make_device(std::string_view self_name, Args &&...args) {
  static_assert(std::is_base_of_v<Device, ConcreteDevice>, "Device must be derived from Device");
  return make_device<ConcreteDevice>(this, self_name, std::forward<Args>(args)...);
}

template <typename ConcreteDevice, typename... Args>
ConcreteDevice *Simulator::make_device(Device::ID parent, std::string_view self_name, Args &&...args) {
  if (auto it = _id_to_device.find(parent); it != _id_to_device.end())
    return make_device<ConcreteDevice>(it->second, self_name, std::forward<Args>(args)...);
  else throw std::runtime_error("Parent device not found");
}

template <typename ConcreteDevice, typename... Args>
ConcreteDevice *Simulator::make_device(Device *parent, std::string_view self_name, Args &&...args) {
  static_assert(std::is_base_of_v<Device, ConcreteDevice>, "ConcreteDevice must be derived from Device");
  // The root element has a trailing /, but internal nodes do not. Avoid doubling // in the fullname.
  const auto fullprefix = parent->descriptor().fullname + (parent->descriptor().fullname.ends_with("/") ? "" : "/");
  const auto basename = std::string(self_name);
  const auto descriptor = Descriptor{.id = _next_id_gen(), .basename = basename, .fullname = fullprefix + basename};
  auto device = new ConcreteDevice(descriptor, std::forward<Args>(args)...);
  // If the device is an event handler, add it as a usable handler in the dispatcher. Not all devices (e.g., this,
  // ClockGovernor) are handlers.
  if constexpr (std::is_base_of_v<EventDispatcher::Handler, ConcreteDevice>) {
    _loop.dispatcher.add_handler(descriptor.id, device);
  }
  return device;
}

template <typename ConcreteClock, typename... Args>
ConcreteClock *Simulator::make_clock(std::string_view self_name, Args &&...args) {
  const auto id = _next_id_gen();
  const std::string basename(self_name);
  const auto fullprefix = clocks.descriptor().fullname;
  const auto desc = Descriptor{.id = _next_id_gen(), .basename = basename, .fullname = fullprefix + "/" + basename};

  auto ret = clocks.make_clock<ConcreteClock>(desc, std::forward<Args>(args)...);
  _id_to_device[id] = std::unique_ptr<Device>{ret};
  return ret;
}

template <typename ConcreteFilter, typename... Args>
ConcreteFilter *Simulator::make_filter(EventDispatcher::DispatchKey DispatchKey, Args &&...args) {
  const auto id = _next_id_gen();
  auto ret = _loop.dispatcher.install_filter<ConcreteFilter>(id, DispatchKey, std::forward<Args>(args)...);
  const auto basename = fmt::format("d{:02x}_t{:02x}", DispatchKey.source.value, (u8)DispatchKey.type);
  const auto descriptor = Descriptor{.id = _next_id_gen(), .basename = basename, .fullname = "/filters/" + basename};
  _id_to_device[id] = new FilterWrapper<ConcreteFilter>(descriptor, ret);
  return ret;
}