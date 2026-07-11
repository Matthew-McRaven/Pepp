#include "core/sim/system.hpp"
#include "./vocab.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "core/sim/systemparser.hpp"
#include "fmt/format.h"

DeviceValue::DeviceValue(Device *dev) : dev(dev) {}

std::string DeviceValue::type_name() const { return fmt::format("Device<{}>", dev->config().compatible); }
AValue::Type DeviceValue::type_code() const { return AValue::Type::Device; }
std::string DeviceValue::describe() const { return fmt::format("<Device {}>", dev->config().fullname); }

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
    ParsingContext ctx;
    auto sys = create_system(casted->fields(), ctx);
    interp->push_psp((u16)interp->allocate_object(SystemValue::make(std::move(sys))));
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
    interp->push_psp((u16)casted->device_count());
  }
}
inline static const NativeOpcode SysDevCount{
    .stack_delta = 0,
    .name = "sys.devcount",
    .h = native_sys_devcount,
};

void register_system_words(Interpreter *p) {
  p->run_on(" var sys drop");
  p->run_on(" var dev.parent drop");
  dict_insert_native(p, SysAlloc, {});
  p->run_on(": sys.walloc cfg @ sys.alloc sys ! sys @ dev.parent ! ;");
  dict_insert_native(p, SysDevCount, {});
  p->run_on(": sys.wdevcount sys @ sys.devcount ;");
}