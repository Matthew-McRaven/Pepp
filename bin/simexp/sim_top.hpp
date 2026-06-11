#pragma once
#include <map>
#include <string>
#include "./event_loop.hpp"

struct Descriptor {
  using ID = u8;
  ID id;
  std::string basename, fullname;
};
struct Device : public EventDispatcher::Handler {
  Device(Descriptor desc) : _desc(desc) {}
  virtual ~Device() = default;
  Descriptor::ID id() const override { return _desc.id; }
  const Descriptor &descriptor() const { return _desc; }

private:
  Descriptor _desc;
};

using IDGenerator = std::function<Descriptor::ID()>;

class Simulator : public Device {
  static constexpr Descriptor _desc{.id = 0, .basename = "/", .fullname = "/"};

public:
  Simulator() : Device(_desc) {}
  ~Simulator() override;
  // Handler interface
  void handle_event(const Event *ev) override;
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
  ConcreteDevice *make_device(Descriptor::ID parent, std::string_view self_name, Args &&...args);
  template <typename ConcreteDevice, typename... Args>
  ConcreteDevice *make_device(Device *parent, std::string_view self_name, Args &&...args);

  template <typename ConcreteDevice, typename... Args> void *make_filter(Args &&...args);

private:
  EventLoop _loop;
  u8 _next_id = 1;
  IDGenerator _next_id_gen = [this]() { return _next_id++; };

  std::map<Descriptor::ID, Device *> _id_to_device;
};

template <typename StopCondition> EventLoop::Status Simulator::run(StopCondition &&stop) { return _loop.run(stop); }

template <typename ConcreteDevice, typename... Args>
ConcreteDevice *Simulator::make_device(std::string_view self_name, Args &&...args) {
  static_assert(std::derived_from<ConcreteDevice, Device>, "Device must be derived from Device");
  return make_device<ConcreteDevice>(this, self_name, std::forward<Args>(args)...);
}

template <typename ConcreteDevice, typename... Args>
ConcreteDevice *Simulator::make_device(Descriptor::ID parent, std::string_view self_name, Args &&...args) {
  if (auto it = _id_to_device.find(parent); it != _id_to_device.end())
    return make_device<ConcreteDevice>(it->second, self_name, std::forward<Args>(args)...);
  else throw std::runtime_error("Parent device not found");
}

template <typename ConcreteDevice, typename... Args>
ConcreteDevice *Simulator::make_device(Device *parent, std::string_view self_name, Args &&...args) {
  static_assert(std::derived_from<ConcreteDevice, Device>, "Device must be derived from Device");
  // The root element has a trailing /, but internal nodes do not. Avoid doubling // in the fullname.
  const auto fullprefix = parent->descriptor().fullname + (parent->descriptor().fullname.ends_with("/") ? "" : "/");
  const auto basename = std::string(self_name);
  const auto descriptor = Descriptor{.id = _next_id_gen(), .basename = basename, .fullname = fullprefix + basename};
  auto device = new ConcreteDevice(descriptor, std::forward<Args>(args)...);

  _loop.dispatcher.register_device(device);
  return device;
}
