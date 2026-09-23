#include "system.hpp"
#include <algorithm>
#include <ranges>
#include <spdlog/spdlog.h>
#include "core/ds/string_compare.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/trace_device.hpp"
#include "core/sim/debugger/trace_recorder.hpp"
#include "core/sim/debugger/tvm_apply_backend.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/devicetree.hpp"
#include "core/sim/systemparser.hpp"

using namespace bits;

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
  _trace_buffer = found == nullptr ? nullptr : &found->buffer();
  if (_trace_buffer != nullptr) bind_recorders(*_trace_buffer);

  populate_scheduler();
}

void System::reset() {
  for (auto dev : *_root)
    if (dev != this) dev->reset();
  settle();

  populate_scheduler();
}

void System::settle() {
  for (auto dev : *_root)
    if (dev != this) dev->settle();
  _scheduler.dirty = true;
}

std::unique_ptr<DeviceSerializer> System::serializer() const { return make_serializer(); }

// Serialization is handled inline in systemparser. Serializer does not transfer ownership of allocated object to
// caller, which is required when initializing a System ex nihilo.
std::unique_ptr<DeviceSerializer> System::make_serializer() { return nullptr; }

Device::Type System::type() const { return Device::Type::SystemRoot | Device::Type::EventSink; }

void System::on_event(Device::ID, const Event &event) {
  if (dynamic_cast<const UpdateSchedule *>(&event) != nullptr) _scheduler.dirty = true;
}


Device::ID System::next_ID() { return _next_ID++; }

Device::IDGenerator System::gen_next_ID() { return _gen_next_ID; }

void System::bind_recorders(tvm::TraceBuffer &tb) {
  // Give the TraceBuffer a helper to convert ids to Traceables. This allows the TraceBuffer to "push" trace
  // enable/disable information to the devices themselves.
  auto finder = [this](Device::ID id) -> Traceable * {
    if (auto dev = find_by_id(id); !dev) return nullptr;
    else return dev->capability<Traceable>();
  };
  tb.set_traceable_finder(finder);
  // Each Traceable must get its own device ID to allow per-ID enables to work.
  for (auto dev : *_root) {
    auto *traceable = dev->capability<Traceable>();
    if (traceable == nullptr) continue;

    traceable->set_recorder(trace::Recorder{&tb, dev->id()});
    // Push the current traced state to the device's local cache.
    traceable->on_traced_changed(tb.traced(dev->id()));
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

// TODO: would prefer if we could avoid dynamic alloc here by returning a stack-allocated iterator of some kind.
std::vector<Device *> System::find_all(std::string_view name) {
  std::vector<Device *> ret;
  if (name.starts_with("/")) {
    if (auto *dev = find_absolute(name); dev != nullptr) ret.push_back(dev);
    return ret;
  }
  // TODO: search should include aliases and path fragments (e.g.) "cpu/regs"
  for (auto *dev : *_root)
    if (dev->config().basename == name) ret.push_back(dev);
  return ret;
}

Device *System::find_by_id(ID id) {
  auto it = _id_to_device.find(id);
  if (it == _id_to_device.end()) return nullptr;
  return it->second ? it->second->device : nullptr;
}

DeviceTree *System::find_tree_by_id(ID id) {
  auto it = _id_to_device.find(id);
  if (it == _id_to_device.end()) return nullptr;
  return it->second;
}

RegisterScan *System::register_scan() { return _hwdbg.get(); }

const RegisterScan *System::register_scan() const { return _hwdbg.get(); }

std::unique_ptr<tvm::Interpreter> System::make_trace_interpreter() {
  auto be = std::make_unique<tvm::TraceApplyBackend>(_buffer_manager, this, _trace_buffer);
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

std::tuple<Device::ID, u64> System::tick() {
  auto &s = _scheduler;
  const size_t n = s.due_tick.size();
  if (n == 0) return {Device::ID{}, Scheduler::MAX_TICK};
  if (s.dirty) refresh_schedules();

  auto lowest_item = std::min_element(s.due_tick.begin(), s.due_tick.end());
  auto lowest_index = std::distance(s.due_tick.begin(), lowest_item);
  auto lowest_tick = *lowest_item;
  if (lowest_tick == Scheduler::MAX_TICK) return {Device::ID{}, lowest_tick};

  s.now = lowest_tick;
  // TODO: need the devices to return a delay.
  u32 delay = 1;
  s.devices[lowest_index].dev->clock_tick(s.due_index[lowest_index], lowest_tick);

  // If clock_tick changed any schedule, the next tick()'s refresh_schedules() corrects this.
  auto [t, idx] = s.devices[lowest_index].schedule.next_clock(lowest_tick, delay);
  s.due_tick[lowest_index] = t;
  s.due_index[lowest_index] = idx;
  return {s.devices[lowest_index].id, lowest_tick};
}

void System::populate_scheduler() {
  auto &s = _scheduler;
  s.now = 0;
  s.dirty = false;
  s.due_tick.clear();
  s.due_index.clear();
  s.devices.clear();

  for (auto *dev : *_root) {
    auto *sink = dev->capability<ClockSink>();
    if (sink == nullptr) continue;
    const auto *src = sink->clock_source();
    if (src == nullptr) throw std::logic_error("System: clock sink " + dev->config().fullname + " has no clock source");
    const auto schedule = src->schedule();
    // index_of divides by period, so a zero-period clock would trap inside tick() rather than here.
    if (schedule.enabled && schedule.period == 0)
      throw std::logic_error("System: clock driving " + dev->config().fullname + " has a period of zero");
    auto [due, index] = Scheduler::next_due(schedule, s.now);
    s.due_tick.push_back(due);
    s.due_index.push_back(index);
    s.devices.push_back(Scheduler::DeviceInfo{.id = dev->id(), .dev = sink, .schedule = schedule});
  }
}

void System::refresh_schedules() {
  auto &s = _scheduler;
  s.dirty = false;
  for (size_t i = 0; i < s.devices.size(); i++) {
    auto &info = s.devices[i];
    const auto schedule = info.dev->clock_source()->schedule();
    // Keep the pending edge asrecomputing it from now could incorrectly skip due to jitter.
    if (schedule == info.schedule) continue;
    if (schedule.enabled && schedule.period == 0)
      throw std::logic_error("System: clock driving device " + std::to_string(info.id.value) + " has a period of zero");
    info.schedule = schedule;
    std::tie(s.due_tick[i], s.due_index[i]) = Scheduler::next_due(schedule, s.now);
  }
}

std::tuple<u64, PulseIndex> System::Scheduler::next_due(const PulseSchedule &sched, u64 after) {
  if (!sched.enabled) return {MAX_TICK, PulseIndex{}};
  return sched.next_clock(after);
}
