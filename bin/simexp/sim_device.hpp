#pragma once

#include <functional>
#include <string>
#include "core/integers.h"
#include "event_dispatch.hpp"

struct Descriptor {
  using ID = u8;
  ID id;
  std::string basename, fullname;
};
struct Device {
  Device(Descriptor desc) : _desc(desc) {}
  virtual ~Device() = default;
  const Descriptor &descriptor() const { return _desc; }

private:
  Descriptor _desc;
};

struct EventHandlingDevice : public Device, EventDispatcher::Handler {
  using Device::Device;
  u8 id() const override { return descriptor().id; }
};

using IDGenerator = std::function<Descriptor::ID()>;