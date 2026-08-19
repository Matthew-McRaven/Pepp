#include "system.hpp"
#include <ranges>
#include <spdlog/spdlog.h>
#include "core/ds/string_compare.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "core/sim/debugger/tvm_apply_backend.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/trace_device.hpp"
#include "core/sim/debugger/trace_recorder.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/devicetree.hpp"
#include "core/sim/systemparser.hpp"

using namespace bits;
consteval void allow_opaque_handle_increment(Device::ID);

System::System(Configuration config)
    : Device(), _config(config), _gen_next_ID([this]() { return next_ID(); }),
      _root(std::make_unique<DeviceTree>(this, nullptr)),
      _buffer_manager(std::make_shared<pepp::bts::BufferManager>()) {
  _config.id = Device::ID{0};
  // Ensure that basename always == fullname, and that the name starts with a /
  if (_config.basename.empty()) _config.basename = "/";
  else if (_config.basename.starts_with("/")) _config.basename = _config.basename;
  else _config.basename = "/" + _config.basename;
  _config.fullname = _config.basename;
  // Ensure we can lookup this device by ID.
  _id_to_device[_config.id] = _root.get();
  _hwdbg = std::make_unique<RegisterScan>(this);
}

void System::initialize(System *sys) { return initialize(); }

void System::initialize() {
  // Trace buffer will already exist prior to init(), but it's actual TB won't be built until after it is initialized.
  // To avoid an initialization-order nightmare, record which device is the TB. We'll use a separate pass to bind
  // Traceables.
  trace::BufferDevice *found = nullptr;
  // Finish initializing devices in a post-order traversal.
  for (auto dev : *_root) {
    if (dev != this) {
      dev->initialize(this);
      // Routing traces between multiple buffers is not supported, so enforce that at most one TB exists.
      if (auto *as_buffer = dev->capability<trace::BufferDevice>(); as_buffer != nullptr) {
        if (found != nullptr) throw std::logic_error("System: more than one trace buffer device");
        found = as_buffer;
      }
    }
  }
  // With all devices initialized, perform another pass to create recorders for each traceable device.
  if (found != nullptr) bind_recorders(found->buffer());
}

void System::reset() {
  for (auto dev : *_root)
    if (dev != this) dev->reset();
}

std::unique_ptr<DeviceSerializer> System::serializer() const { return make_serializer(); }

// Serialization is handled inline in systemparser. Serializer does not transfer ownership of allocated object to
// caller, which is required when initializing a System ex nihilo.
std::unique_ptr<DeviceSerializer> System::make_serializer() { return nullptr; }

Device::ID System::next_ID() { return _next_ID++; }

Device::IDGenerator System::gen_next_ID() { return _gen_next_ID; }

void System::bind_recorders(tvm::TraceBuffer &tb) {
  // Each Traceable must get its own device ID to allow per-ID enables to work.
  for (auto dev : *_root) {
    auto *traceable = dev->capability<Traceable>();
    if (traceable == nullptr) continue;

    traceable->set_recorder(trace::Recorder{&tb, dev->id()});
    // Try to select register banks and CSRs by filtering based on their size.
    // For small targets, prefer to pass addresses/offsets via the trace's code rather than the trace's data stream. A
    // system that knows better can call set_address_in_payload() after initialize().
    if (auto *target = dev->capability<Target>(); target != nullptr) {
      const bool wide = size_inclusive(target->span()) > tvm::TraceBuffer::DEFAULT_NARROW_TARGET_BYTES;
      tb.set_address_in_payload(dev->id(), wide);
    }
  }
}

void System::make_deferred(DeferredDevice ctor) { _deferred_constructors.push_back(std::move(ctor)); }

Device *System::find_relative(std::string_view name, std::string_view parent) {
  if (name.starts_with("/")) return find_absolute(name);
  else return find_absolute(child_name(parent, name));
}

Device *System::find_by_id(ID id) {
  auto it = _id_to_device.find(id);
  if (it == _id_to_device.end()) return nullptr;
  return it->second ? it->second->device : nullptr;
}

RegisterScan *System::register_scan() { return _hwdbg.get(); }

const RegisterScan *System::register_scan() const { return _hwdbg.get(); }

std::unique_ptr<tvm::Interpreter> System::make_trace_interpreter() {
  auto be = std::make_unique<tvm::ApplyBackend>(_buffer_manager, this);
  return std::make_unique<tvm::Interpreter>(_buffer_manager, std::move(be));
}

std::shared_ptr<pepp::bts::BufferManager> System::buffer_manager() { return _buffer_manager; }

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
