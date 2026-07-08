#include "objheap.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "fmt/format.h"

AValue::~AValue() = default;

void native_value_typename(Interpreter *interp) {
  u16 idx = interp->pop_psp<u16>();
  auto obj = interp->get_object(idx);
  std::cout << (obj ? obj->type_name() : "null") << std::endl;
}

void native_value_describe(Interpreter *interp) {
  u16 idx = interp->pop_psp<u16>();
  auto obj = interp->get_object(idx);
  std::cout << (obj ? obj->describe() : "null") << std::endl;
}

void make_system(Interpreter *interp) {
  auto sys_value = SystemValue::make();
  interp->push_psp((u16)interp->allocate_object(sys_value));
}

void system_devcount(Interpreter *interp) {
  auto idx = interp->pop_psp<u16>();
  auto obj = interp->get_object(idx);
  auto casted = std::dynamic_pointer_cast<SystemValue>(obj);
  if (!casted) {
    std::cerr << "No system at index " << idx << std::endl;
    interp->push_psp((u16)0);
  } else interp->push_psp(casted->devcount());
}

std::shared_ptr<SystemValue> SystemValue::make() {
  auto inner = std::make_shared<System>();
  std::shared_ptr<SystemValue> ret = std::make_shared<SystemValue>();
  ret->_value = inner;
  return ret;
}

u16 SystemValue::devcount() const {
  if (!_value) return 0;
  auto dt = _value->root();
  return std::distance(dt->begin(), dt->end());
}

void dump_objects(Interpreter *interp) {
  auto heap = interp->get_object_heap();
  for (const auto &[id, obj] : heap)
    std::cout << fmt::format("[{:04x}](*{}):  {}\n", id, obj->type_name(), obj->describe());
}

void config_set_field(Interpreter *interp, u16 addr_name, u16 addr_value) {
  const char *name_ptr = (const char *)&interp->memory[addr_name];
  const char *value_ptr = (const char *)&interp->memory[addr_value];
  std::string_view name(name_ptr), value(value_ptr);
  auto cfg_idx = interp->pop_psp<u16>();
  auto cfg_obj = interp->get_object(cfg_idx);
  auto cfg_ptr = std::dynamic_pointer_cast<Configuration>(cfg_obj);
  if (!cfg_ptr) {
    std::cerr << "No config at index " << cfg_idx << std::endl;
    return;
  }
  cfg_ptr->set_field(name, value);
}

std::string DenseConfigValue::get_field(std::string_view name) const { return ""; }

void DenseConfigValue::set_field(std::string_view name, std::string_view value) {
  std::cout << "Setting field '" << name << "' to '" << value << "'\n";
}

std::shared_ptr<DenseConfigValue> DenseConfigValue::make() { return std::make_shared<DenseConfigValue>(); }

void make_dense_config(Interpreter *interp) {
  auto value = DenseConfigValue::make();
  interp->push_psp((u16)interp->allocate_object(value));
}

void make_dense_device(Interpreter *interp) {
  auto cfg_idx = interp->pop_psp<u16>();
  auto cfg_obj = interp->get_object(cfg_idx);
  auto cfg_ptr = std::dynamic_pointer_cast<DenseConfigValue>(cfg_obj);
  auto sys_idx = interp->pop_psp<u16>();
  auto sys_obj = interp->get_object(sys_idx);
  auto sys_ptr = std::dynamic_pointer_cast<SystemValue>(sys_obj);
  if (!cfg_ptr) {
    std::cerr << "No config at index " << cfg_idx << std::endl;
    interp->push_psp((u16)0);
    return;
  }
  if (!sys_ptr) {
    std::cerr << "No system at index " << sys_idx << std::endl;
    interp->push_psp((u16)0);
    return;
  }
  auto device_value = DenseDeviceValue::make(sys_ptr, cfg_ptr);
  interp->push_psp((u16)interp->allocate_object(device_value));
}

std::shared_ptr<DenseDeviceValue> DenseDeviceValue::make(std::shared_ptr<SystemValue> sys_val,
                                                         std::shared_ptr<DenseConfigValue> cfg_val) {

  auto sys = sys_val->value();
  auto cfg = cfg_val->value();
  auto ret = std::make_shared<DenseDeviceValue>();
  ret->_sys = sys;
  ret->_value = sys->make_device<Dense>(sys.get(), Dense::Configuration{cfg});
  return ret;
}

void register_native_heap_fns(Interpreter *p) {
  dict_insert_native(p, ValueTypeName, {});
  dict_insert_native(p, ValueDescribe, {});
  dict_insert_native(p, MakeSystem, {});
  dict_insert_native(p, SystemDevCount, {});
  dict_insert_native(p, MakeDenseConfig, {});
  dict_insert_native(p, MakeDenseDevice, {});
  dict_insert_native(p, DumpObjects, {});
  const u16 arg1_spad = p->cb.here;
  p->cb.here += 32;
  NativeOpcode PushA1 = {
      .stack_delta = 2,
      .name = "a1",
      .h = [arg1_spad](Interpreter *i) { push_constant(i, arg1_spad); },
  };
  dict_insert_native(p, PushA1, {});
  const u16 arg2_spad = p->cb.here;
  p->cb.here += 32;
  NativeOpcode PushA2 = {
      .stack_delta = 2,
      .name = "a2",
      .h = [arg2_spad](Interpreter *i) { push_constant(i, arg2_spad); },
  };
  dict_insert_native(p, PushA2, {});

  NativeOpcode SetField = {
      .stack_delta = -2,
      .name = "cfg.set",
      .h = [arg1_spad, arg2_spad](Interpreter *i) { config_set_field(i, arg1_spad, arg2_spad); },
  };
  auto h_setfield = dict_insert_native(p, SetField, {});
  p->buffered.emplace_back(": cfg.set word a1 cmove0 word a2 cmove0 cfg.set ;\n");
}
