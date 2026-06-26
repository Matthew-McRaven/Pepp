#pragma once

#include <functional>
#include <string>
#include "core/ds/opaque_handle.hpp"
#include "core/integers.h"

struct Device {
  using ID = pepp::OpaqueHandle<struct DeviceID, u8>;
  using IDGenerator = std::function<Device::ID()>;
  struct Descriptor {
    ID id = Device::ID{0};
    std::string basename = "", fullname = "", compatible = "";
    std::string child_name(std::string_view child_basename) const;
  };

  Device(Descriptor desc) : _desc(desc) {}
  virtual ~Device() = default;
  const Descriptor &descriptor() const { return _desc; }

private:
  Descriptor _desc;
};
