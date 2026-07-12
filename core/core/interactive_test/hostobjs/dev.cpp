#include <nlohmann/json.hpp>
#include "./vocab.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "core/sim/systemparser.hpp"
#include "fmt/format.h"

void native_dev_alloc(Interpreter *p) {
  auto cfg_idx = p->pop_psp<u16>();
  auto cfg = p->get_object(cfg_idx);
  auto cfg_casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
  if (!cfg_casted) throw std::runtime_error("Invalid configuration object index");

  auto par_idx = p->pop_psp<u16>();
  auto par = p->get_object(par_idx);
  auto casted_par = std::dynamic_pointer_cast<DeviceValue>(par);
  if (!casted_par) throw std::runtime_error("Invalid parent device object index");

  auto sys_idx = p->pop_psp<u16>();
  auto sys_val = p->get_object(sys_idx);
  auto casted_sys = std::dynamic_pointer_cast<SystemValue>(sys_val);
  if (!casted_sys) throw std::runtime_error("Invalid system object index");

  auto device = create_device(cfg_casted->fields(), casted_sys->sys.get(), casted_par->dev);
  p->push_psp(p->allocate_object(DeviceValue::make(device)));
}

inline static const NativeOpcode DevAlloc{
    .stack_delta = -4,
    .name = "dev.alloc",
    .h = native_dev_alloc,
};

void native_dev_basename(Interpreter *p) {
  auto buf = p->pop_psp<u16>();
  auto dev_idx = p->pop_psp<u16>();
  auto dev_val = p->get_object(dev_idx);
  auto casted_dev = std::dynamic_pointer_cast<DeviceValue>(dev_val);
  if (!casted_dev) throw std::runtime_error("Invalid device object index");

  auto basename = casted_dev->dev->config().basename;
  p->write(buf, std::span<const u8>((const u8 *)basename.data(), basename.size()));
  p->write(0, (u16)buf + basename.size());
  p->push_psp<u16>(basename.size());
}
inline static const NativeOpcode DevBaseName{
    .stack_delta = -2,
    .name = "dev.basename",
    .h = native_dev_basename,
};
void native_dev_fullname(Interpreter *p) {
  auto buf = p->pop_psp<u16>();
  auto dev_idx = p->pop_psp<u16>();
  auto dev_val = p->get_object(dev_idx);
  auto casted_dev = std::dynamic_pointer_cast<DeviceValue>(dev_val);
  if (!casted_dev) throw std::runtime_error("Invalid device object index");

  auto basename = casted_dev->dev->config().fullname;
  p->write(buf, std::span<const u8>((const u8 *)basename.data(), basename.size()));
  p->write(0, (u16)buf + basename.size());
  p->push_psp<u16>(basename.size());
}
inline static const NativeOpcode DevFullName{
    .stack_delta = -2,
    .name = "dev.fullname",
    .h = native_dev_fullname,
};

void native_dev_type(Interpreter *p) {
  auto dev_idx = p->pop_psp<u16>();
  auto dev_val = p->get_object(dev_idx);
  auto casted_dev = std::dynamic_pointer_cast<DeviceValue>(dev_val);
  if (!casted_dev) throw std::runtime_error("Invalid device object index");

  auto type = casted_dev->dev->type();
  p->push_psp<u16>((u16)type);
}
inline static const NativeOpcode DevType{
    .stack_delta = 0,
    .name = "dev.type",
    .h = native_dev_type,
};

void native_dev_id(Interpreter *p) {
  auto dev_idx = p->pop_psp<u16>();
  auto dev_val = p->get_object(dev_idx);
  auto casted_dev = std::dynamic_pointer_cast<DeviceValue>(dev_val);
  if (!casted_dev) throw std::runtime_error("Invalid device object index");

  p->push_psp<u16>(casted_dev->dev->id().value);
}
inline static const NativeOpcode DevID{
    .stack_delta = 0,
    .name = "dev.id",
    .h = native_dev_id,
};

void register_device_words(Interpreter *p) {
  p->run_on("var dev drop");
  auto op_alloc = p->register_native(DevAlloc);
  p->run_on(fmt::format(": dev.alloc sys @ dev.parent @ cfg @ op 0x{:04x} dev ! ;", op_alloc));
  auto op_basename = p->register_native(DevBaseName);
  p->run_on(fmt::format(": dev.basename t1 dev @ t1 op 0x{:04x} ;", op_basename));
  auto op_fullname = p->register_native(DevFullName);
  p->run_on(fmt::format(": dev.fullname t1 dev @ t1 op 0x{:04x} ;", op_fullname));
  auto op_type = p->register_native(DevType);
  p->run_on(fmt::format(": dev.type dev @ op 0x{:04x} ;", op_type));
  auto op_id = p->register_native(DevID);
  p->run_on(fmt::format(": dev.id dev @ op 0x{:04x} ;", op_id));
}