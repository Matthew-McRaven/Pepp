#pragma once

#include <functional>
#include <string>
#include "core/ds/opaque_handle.hpp"
#include "core/integers.h"

struct Device {
  using ID = pepp::OpaqueHandle<struct DeviceID, u8>;
  struct Descriptor {
    ID id;
    std::string basename, fullname;
  };

  Device(Descriptor desc) : _desc(desc) {}
  virtual ~Device() = default;
  const Descriptor &descriptor() const { return _desc; }

private:
  Descriptor _desc;
};

using IDGenerator = std::function<Device::ID()>;