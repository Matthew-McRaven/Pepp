#include "core/sim/debugger/trace_device.hpp"
#include <stdexcept>
#include "core/sim/system.hpp"

namespace trace {

BufferDevice::BufferDevice(Configuration cfg) : _config(std::move(cfg)) {
  // Has no JSON representation, so always skip serialization.
  _config.skip_serialize = true;
  if (_config.compatible.empty()) _config.compatible = compatible;
}

void BufferDevice::initialize(System *sys) {
  _tb = std::make_unique<tvm::TraceBuffer>(sys->buffer_manager(), _config.ring_size);
}

void BufferDevice::reset() {
  if (_tb) _tb->clear();
}

tvm::TraceBuffer &BufferDevice::buffer() {
  if (!_tb) [[unlikely]]
    throw std::logic_error("trace::BufferDevice: buffer requested before initialize()");
  return *_tb;
}

const tvm::TraceBuffer &BufferDevice::buffer() const {
  if (!_tb) [[unlikely]]
    throw std::logic_error("trace::BufferDevice: buffer requested before initialize()");
  return *_tb;
}

std::unique_ptr<DeviceSerializer> BufferDevice::serializer() const {
  // Should never be called because we set skip_serialize = true in the constructor.
  // Throw an error to force me to fix the bug otherwise.
  throw std::logic_error("trace::BufferDevice is synthetic and has no serialized form");
}

} // namespace trace
