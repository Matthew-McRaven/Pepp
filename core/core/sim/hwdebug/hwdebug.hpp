#pragma once
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"

// What is the data type stored at the named location?
enum class DebugType : u8 {
  u1 = 1,
  u8 = 2,
  u16 = 3,
  i8 = 4,
  i16 = 5,
};

// Corresponds to an AddressSpan in some Target.
struct NamedLocation {
  DebugType type;
  Device::ID target;
  std::string name;
  Address offset;
};

// The value is resolved at the time expose(...) is called and never changes.
// Useful for hardware properties like an ISA version or device id.
struct NamedConstant {
  DebugType type;
  Device::ID target;
  // TODO: should be an abstract value-ish class
  u16 value;
  std::string name;
};

using Named = std::variant<NamedLocation, NamedConstant>;

// A class that acts a bit like the scanchain of a JTAG debugger, allowing named entries to be exposed
// Devices call expose(...) as part of scan_debug_hardware(...), registering locations which can be read and written by
// a debugger.
class HWDebug {
public:
  HWDebug(System *sys) : _sys(sys) {}
  void expose(const Named &n);
  // TODO: should return an abstract value.
  u16 read(const Named &n);
  std::optional<Named> find(std::string_view name);

private:
  System *_sys;
  std::unordered_map<Device::ID, std::list<Named>> _exposed;
};
