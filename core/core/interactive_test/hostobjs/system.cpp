#include "core/sim/system.hpp"
#include <nlohmann/json.hpp>
#include "./vocab.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "core/sim/systemparser.hpp"
#include "fmt/format.h"

DeviceValue::DeviceValue(Device *dev) : dev(dev) {}

std::string DeviceValue::type_name() const { return fmt::format("Device<{}>", dev->config().compatible); }
AValue::Type DeviceValue::type_code() const { return AValue::Type::Device; }
std::string DeviceValue::describe() const { return fmt::format("<Device {}>", dev->config().fullname); }

void *DeviceValue::data() const { return static_cast<void *>(dev); }

std::shared_ptr<DeviceValue> DeviceValue::make(Device *v) { return std::make_shared<DeviceValue>(v); }

SystemValue::SystemValue(std::unique_ptr<System> sys) : DeviceValue(sys.get()), sys(std::move(sys)) {}

std::shared_ptr<DeviceValue> SystemValue::make(std::unique_ptr<System> sys) {
  return std::make_shared<SystemValue>(std::move(sys));
}
u16 SystemValue::device_count() const {
  auto b = sys->root()->begin(), e = sys->root()->end();
  return std::distance(b, e);
}

void native_sys_alloc(Interpreter *interp) {
  auto cfg_idx = interp->pop_psp<u16>();
  auto cfg = interp->get_object(cfg_idx);
  if (!cfg) throw std::runtime_error("Invalid configuration object index");
  if (auto casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
      cfg->type_code() != AValue::Type::InteractiveConfig || casted == nullptr)
    throw std::runtime_error("Object is not a configuration object");
  else {
    auto sys = create_system(casted->fields());
    interp->push_psp<u16>(interp->allocate_object(SystemValue::make(std::move(sys))));
  }
}
inline static const NativeOpcode SysAlloc{
    .stack_delta = 0,
    .name = "sys.alloc",
    .h = native_sys_alloc,
};

void native_sys_devcount(Interpreter *interp) {
  auto sys_idx = interp->pop_psp<u16>();
  auto sys_val = interp->get_object(sys_idx);
  if (!sys_val) throw std::runtime_error("Invalid system object index");
  if (auto casted = std::dynamic_pointer_cast<SystemValue>(sys_val);
      sys_val->type_code() != AValue::Type::Device || casted == nullptr)
    throw std::runtime_error("Object is not a system object");
  else {
    interp->push_psp<u16>(casted->device_count());
  }
}
inline static const NativeOpcode SysDevCount{
    .stack_delta = 0,
    .name = "sys.devcount",
    .h = native_sys_devcount,
};

void native_sys_init(Interpreter *interp) {
  auto sys_idx = interp->pop_psp<u16>();
  auto sys_val = interp->get_object(sys_idx);
  if (!sys_val) throw std::runtime_error("Invalid system object index");
  if (auto casted = std::dynamic_pointer_cast<SystemValue>(sys_val);
      sys_val->type_code() != AValue::Type::Device || casted == nullptr)
    throw std::runtime_error("Object is not a system object");
  else casted->sys->initialize();
}
inline static const NativeOpcode SysInit{
    .stack_delta = -2,
    .name = "sys.init",
    .h = native_sys_init,
};

// ( dev_id sys_idx -- dev_idx ) given a device ID and a system object index, push the heap index of that device onto
// the stack.
void native_sys_id(Interpreter *interp) {
  auto sys_idx = interp->pop_psp<u16>();
  auto dev_id = interp->pop_psp<u16>();
  auto sys_val = interp->get_object(sys_idx);
  if (!sys_val) throw std::runtime_error("Invalid system object index");
  if (auto casted = std::dynamic_pointer_cast<SystemValue>(sys_val);
      sys_val->type_code() != AValue::Type::Device || casted == nullptr)
    throw std::runtime_error("Object is not a system object");
  else {
    auto dev = casted->sys->find_by_id((Device::ID)dev_id);
    if (!dev) throw std::runtime_error(fmt::format("Device with ID {} not found in system", dev_id));
    auto maybe_value = interp->object_from_data(dev);
    if (!maybe_value) {
      interp->push_psp<u16>(interp->allocate_object(DeviceValue::make(dev)));
    } else {
      interp->push_psp<u16>(maybe_value.value());
    }
  }
}
inline static const NativeOpcode SysId{.stack_delta = -4, .name = "sys.id", .h = native_sys_id};

// (sys_idx -- )
void native_sys_json(Interpreter *interp) {
  auto sys_idx = interp->pop_psp<u16>();
  auto sys_val = interp->get_object(sys_idx);
  if (!sys_val) throw std::runtime_error("Invalid system object index");
  if (auto casted = std::dynamic_pointer_cast<SystemValue>(sys_val);
      sys_val->type_code() != AValue::Type::Device || casted == nullptr)
    throw std::runtime_error("Object is not a system object");
  else {
    nlohmann::json obj;
    serialize_system(casted->sys.get(), obj);
    interp->output->write(fmt::format("{}\n", obj.dump(2)));
  }
}
inline static const NativeOpcode SysJson{.stack_delta = -2, .name = "sys.json", .h = native_sys_json};

void register_system_words(Interpreter *p) {
  p->run_on(" var sys drop");
  p->run_on(" var dev.parent drop");
  auto op_alloc = p->register_native(SysAlloc);
  p->run_on(fmt::format(": sys.alloc cfg @ op 0x{:04x} sys ! sys @ dev.parent ! ;", op_alloc));
  auto op_devcount = p->register_native(SysDevCount);
  p->run_on(fmt::format(": sys.devcount sys @ op 0x{:04x} ;", op_devcount));
  auto op_init = p->register_native(SysInit);
  p->run_on(fmt::format(": sys.init sys @ op 0x{:04x} ;", op_init));
  auto op_id = p->register_native(SysId);
  p->run_on(fmt::format(": sys.device sys @ op 0x{:04x} ;", op_id));
  auto op_json = p->register_native(SysJson);
  p->run_on(fmt::format(": sys.json sys @ op 0x{:04x} ;", op_json));
}