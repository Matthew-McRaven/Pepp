#include "system.hpp"
#include <ranges>
#include <spdlog/spdlog.h>
#include "core/ds/string_compare.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "core/sim/devicetree.hpp"

using namespace bits;
consteval void allow_opaque_handle_increment(Device::ID);

System::System()
    : Device(), _gen_next_ID([this]() { return next_ID(); }), _root(std::make_unique<DeviceTree>(this, nullptr)) {}

void System::initialize(System *sys) { return initialize(); }

void System::initialize() {
  // Finish initializing devices in a post-order traversal.
  for (auto dev : *_root)
    if (dev != this) dev->initialize(this);
}

Device::ID System::next_ID() { return _next_ID++; }

Device::IDGenerator System::gen_next_ID() { return _gen_next_ID; }

void System::set_buffer(trace::Buffer *buffer) { throw std::logic_error("Unimplemented"); }

Device *System::find_relative(std::string_view name, std::string_view parent) {
  if (name.starts_with("/")) return find_absolute(name);
  else return find_absolute(child_name(parent, name));
}

Device *System::find_absolute(std::string_view name) {
  DeviceTree *root = _root.get();
  auto ptr = (*root) | std::views::filter([&name](Device *dt) { return dt->config().fullname == name; });
  auto count = std::ranges::distance(ptr);
  if (count > 1) {
    SPDLOG_WARN("System::find_absolute: multiple devices found with name {}", name);
    return nullptr;
  } else if (count == 0) return nullptr;
  else return *ptr.begin();
}
