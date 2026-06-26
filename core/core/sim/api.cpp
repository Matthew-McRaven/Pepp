#include "core/sim/api.hpp"

Device *Device::capability(Device::Type t) {
  using namespace bits;
  if (any(type() & t)) return this;
  else return nullptr;
}
