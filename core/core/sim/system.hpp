
#include <queue>
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
#include "core/sim/hwdebug/hwdebug.hpp"

/*struct Scheduler {
  virtual tick::Recipient *next(tick::Type current) = 0;
  virtual void schedule(tick::Recipient *listener, tick::Type startingOn) = 0;
  virtual void reschedule(device::ID device, tick::Type startingOn) = 0;
};*/

namespace trace {
class Buffer;
}

struct DeferredDevice {
  // Do not capture pointer to device in case the device moves during construction.
  // After the device is constructed it will be in a stable location inside the system.
  // We can find_by_id and cast to the correct type.
  Device::ID parent;
  // Rather than forcing ctor to capture sys, we pass it in as an argument.
  // The caller of this class will always be a system, so it should be trivial to pass in this :)
  // The signature covers the parameters needed to construct a CPU's register bank without having any captures.
  // More complex cases may need captures with the onus on the person who constructed this DeferredDevice to properly
  // manage lifetimes and the possibility of the parent device having been moved.
  std::function<void(System *, Device::ID)> ctor;
  // Syntactic sugar to call our function pointer with the correct argument.
  void operator()(System *sys) { ctor(sys, parent); }
};

class System : public Device {
public:
  struct Configuration : public Device::Configuration {
    // No additional configuration for now.
  };
  static constexpr Device::Type TypeMask = Device::Type::SystemRoot;
  inline static std::string compatible = "system,root";
  System(Configuration config = Configuration{{.basename = "/", .compatible = System::compatible}});
  ~System() = default;
  System(const System &) = delete;
  System(System &&) = delete;
  System &operator=(const System &) = delete;
  System &operator=(System &&) = delete;
  // System* will be ignored and call the 0-artity variant.
  void initialize(System *sys) override;
  // Iterate over all devices in the tree and call initialize on each of them.
  void initialize();
  // Return a ptr to a type which can convert this object to/from JSON.
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  const Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }

  Device::ID next_ID();
  Device::IDGenerator gen_next_ID();

  void set_buffer(trace::Buffer *buffer);

  // Create a device that is a child of the root (this system)
  template <typename ConcreteDevice, typename ConcreteConfig, typename... Args>
    requires std::same_as<std::remove_cvref_t<ConcreteConfig>, typename ConcreteDevice::Configuration>
  ConcreteDevice *make_device(ConcreteConfig &&cfg, Args &&...args);
  // Create children under a given device.
  template <typename ConcreteDevice, typename ConcreteConfig, typename... Args>
    requires std::same_as<std::remove_cvref_t<ConcreteConfig>, typename ConcreteDevice::Configuration>
  ConcreteDevice *make_device(Device::ID parent, ConcreteConfig &&cfg, Args &&...args);
  template <typename ConcreteDevice, typename ConcreteConfig, typename... Args>
    requires std::same_as<std::remove_cvref_t<ConcreteConfig>, typename ConcreteDevice::Configuration>
  ConcreteDevice *make_device(Device *parent, ConcreteConfig &&cfg, Args &&...args);

  // Perform an action after finishing the current make_device call. This is highly useful for creating a child device
  // after it's parent has been fully constructed, e.g., make a register bank after creating the CPU. In the case of
  // multiple make_deferred calls, the are executed in FIFO order.
  void make_deferred(DeferredDevice ctor);

  // Return a pointer to a device by name, or nullptr if not found.
  // While these could be free function operating on DeviceTrees, it's more convenient for 2-stage device initialization
  // for the System to provide the lookup.
  Device *find_absolute(std::string_view name);
  // Combine relative_to and name to form an absolute path and call find_absolute. Names starting with '/' are treated
  // as absolute by default and will not be combined with parent.
  Device *find_relative(std::string_view name, std::string_view parent);

  // Given a device ID, return a pointer to the device or nullptr if not found.
  Device *find_by_id(Device::ID id);

  DeviceTree *root() { return _root.get(); }
  const DeviceTree *root() const { return _root.get(); }

  HWDebug *hw_debugger();
  const HWDebug *hw_debugger() const;

private:
  Configuration _config{{.basename{"/"}, .fullname{"/"}}};
  Device::ID _next_ID = Device::ID(1);
  Device::IDGenerator _gen_next_ID = [] { return Device::ID(0); };
  static inline Device::Configuration _root_desc{.basename{"/"}, .fullname{"/"}};
  std::unique_ptr<DeviceTree> _root = nullptr;
  std::unique_ptr<HWDebug> _hwdbg = nullptr;
  std::map<Device::ID, DeviceTree *> _id_to_device;
  // Prevent infinite recursion on make_device while doing deferred initialization.
  // The top level call to make_device sets this flag to true, and that top level call will pull all of the work out of
  // the ctor list. While ctors may themselves enqueue more deferred ctors, they will be processed within the top-level
  // call in a FIFO order.
  bool _doing_deferred = false;
  std::deque<DeferredDevice> _deferred_constructors;
};

template <typename ConcreteDevice, typename ConcreteConfig, typename... Args>
  requires std::same_as<std::remove_cvref_t<ConcreteConfig>, typename ConcreteDevice::Configuration>
ConcreteDevice *System::make_device(Device *parent, ConcreteConfig &&cfg, Args &&...args) {
  const auto id = parent->id();
  if (auto it = _id_to_device.find(id); it != _id_to_device.end())
    return make_device<ConcreteDevice>(id, cfg, std::forward<Args>(args)...);
  else throw std::runtime_error("Parent device not found");
}

template <typename ConcreteDevice, typename ConcreteConfig, typename... Args>
  requires std::same_as<std::remove_cvref_t<ConcreteConfig>, typename ConcreteDevice::Configuration>
ConcreteDevice *System::make_device(Device::ID parent_id, ConcreteConfig &&cfg, Args &&...args) {

  auto device_tree = _id_to_device.find(parent_id);
  if (device_tree == _id_to_device.end()) throw std::runtime_error("Parent device not found");
  auto &parent = device_tree->second->device;
  static_assert(std::is_base_of_v<Device, ConcreteDevice>, "ConcreteDevice must be derived from Device");
  cfg.id = next_ID();
  cfg.fullname = child_name(parent->config().fullname, cfg.basename);
  auto device = std::make_unique<ConcreteDevice>(cfg, std::forward<Args>(args)...);
  auto ptr = device.get();
  { // Force child_dt to go out of scope after move.
    auto child_dt = std::make_unique<DeviceTree>(std::move(device), device_tree->second);
    _id_to_device[cfg.id] = child_dt.get();
    device_tree->second->children.push_back(std::move(child_dt));
  }
  // If we are not already processing deferred ctors, then process all of the deferred ctors now.
  // This recurses, because ctor always calls make_device. And in theory, this can recurse arbitrarily,
  // because a deferred device could depend on a further deferred device. Replace recursion with a queue, which also has
  // the nice benefit of constructing in FIFO order.
  if (!_doing_deferred && !_deferred_constructors.empty()) {
    _doing_deferred = true;
    while (!_deferred_constructors.empty()) {
      auto ctor = std::move(_deferred_constructors.front());
      _deferred_constructors.pop_front();
      ctor(this);
    }
    _doing_deferred = false;
  }
  return ptr;
}

template <typename ConcreteDevice, typename ConcreteConfig, typename... Args>
  requires std::same_as<std::remove_cvref_t<ConcreteConfig>, typename ConcreteDevice::Configuration>
ConcreteDevice *System::make_device(ConcreteConfig &&cfg, Args &&...args) {

  static_assert(std::is_base_of_v<Device, ConcreteDevice>, "Device must be derived from Device");
  // Avoid looking up this device ID, when we already have it stored in _config.
  return make_device<ConcreteDevice>(_config.id, cfg, std::forward<Args>(args)...);
}
