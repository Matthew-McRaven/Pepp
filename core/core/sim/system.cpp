#include "system.hpp"
#include "core/math/bitmanip/enums.hpp"
using namespace bits;
consteval void allow_opaque_handle_increment(Device::ID);

System::System()
    : Device(_root_desc, ID{0}), _gen_next_ID([this]() { return next_ID(); }),
      _root(std::make_unique<DeviceTree>(this, nullptr)) {}

Device::ID System::next_ID() { return _next_ID++; }

Device::IDGenerator System::gen_next_ID() { return _gen_next_ID; }

void System::set_buffer(trace::Buffer *buffer) { throw std::logic_error("Unimplemented"); }
