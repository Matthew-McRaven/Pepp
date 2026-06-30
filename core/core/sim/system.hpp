/*
 * /Copyright (c) 2024-2025. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <map>
#include <memory>
#include <vector>
#include "core/sim/api/device.hpp"
#include "core/sim/devicetree.hpp"

/*struct Scheduler {
  virtual tick::Recipient *next(tick::Type current) = 0;
  virtual void schedule(tick::Recipient *listener, tick::Type startingOn) = 0;
  virtual void reschedule(device::ID device, tick::Type startingOn) = 0;
};*/

namespace trace {
class Buffer;
}

class System : public Device {
public:
  struct Configuration : public Device::Configuration {
    // No additional configuration for now.
  };
  static constexpr Device::Type TypeMask = Device::Type::SystemRoot;
  System();
  ~System() = default;
  System(const System &) = delete;
  System(System &&) = delete;
  System &operator=(const System &) = delete;
  System &operator=(System &&) = delete;
  // System* will be ignored and call the 0-artity variant.
  void initialize(System *sys) override;
  // Iterate over all devices in the tree and call initialize on each of them.
  void initialize();

  const Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _self_id; }

  Device::ID next_ID();
  Device::IDGenerator gen_next_ID();

  void set_buffer(trace::Buffer *buffer);

  // Create a device that is a child of the root (this system)
  template <typename ConcreteDevice, typename... Args> ConcreteDevice *make_device(Args &&...args);
  // Create children under a given device.
  template <typename ConcreteDevice, typename... Args> ConcreteDevice *make_device(Device::ID parent, Args &&...args);
  template <typename ConcreteDevice, typename... Args> ConcreteDevice *make_device(Device *parent, Args &&...args);

  // Return a pointer to a device by name, or nullptr if not found.
  // While these could be free function operating on DeviceTrees, it's more convenient for 2-stage device initialization
  // for the System to provide the lookup.
  Device *find_absolute(std::string_view name);
  // Combine relative_to and name to form an absolute path and call find_absolute. Names starting with '/' are treated
  // as absolute by default and will not be combined with parent.
  Device *find_relative(std::string_view name, std::string_view parent);

private:
  Configuration _config{{.basename = "/", .fullname = "/"}};
  Device::ID _self_id = Device::ID(0);
  Device::ID _next_ID = Device::ID(1);
  Device::IDGenerator _gen_next_ID = [] { return Device::ID(0); };
  static inline Device::Configuration _root_desc{.basename = "/", .fullname = "/"};
  std::unique_ptr<DeviceTree> _root = nullptr;
  std::map<Device::ID, DeviceTree *> _id_to_device;
};

template <typename ConcreteDevice, typename... Args>
ConcreteDevice *System::make_device(Device *parent, Args &&...args) {
  const auto id = parent->id();
  if (auto it = _id_to_device.find(id); it != _id_to_device.end())
    return make_device<ConcreteDevice>(id, std::forward<Args>(args)...);
  else throw std::runtime_error("Parent device not found");
}

template <typename ConcreteDevice, typename... Args>
ConcreteDevice *System::make_device(Device::ID parent_id, Args &&...args) {
  auto device_tree = _id_to_device.find(parent_id);
  if (device_tree == _id_to_device.end()) throw std::runtime_error("Parent device not found");
  auto &parent = device_tree->second->device;
  static_assert(std::is_base_of_v<Device, ConcreteDevice>, "ConcreteDevice must be derived from Device");
  auto device = std::make_unique<ConcreteDevice>(next_ID(), std::forward<Args>(args)...);
  auto ptr = device.get();
  { // Force child_dt to go out of scope after move.
    auto child_dt = std::make_unique<DeviceTree>(std::move(device), device_tree->second);
    device_tree->second->children.push_back(std::move(child_dt));
  }
  return ptr;
}

template <typename ConcreteDevice, typename... Args> ConcreteDevice *System::make_device(Args &&...args) {
  static_assert(std::is_base_of_v<Device, ConcreteDevice>, "Device must be derived from Device");
  return make_device<ConcreteDevice>(this, std::forward<Args>(args)...);
}
