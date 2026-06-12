
#pragma once

#include "./event_dispatch.hpp"
#include "./sim_device.hpp"

struct EventHandlingDevice : public Device, EventDispatcher::Handler {
  using Device::Device;
  Device::ID id() const override { return descriptor().id; }
};