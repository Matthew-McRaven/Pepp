#include "objheap.hpp"
#include "core/interactive_test/dict.hpp"
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

void register_native_heap_fns(Interpreter *p) {
  dict_insert_native(p, ValueTypeName, {});
  dict_insert_native(p, ValueDescribe, {});
  dict_insert_native(p, MakeSystem, {});

  dict_insert_native(p, DumpObjects, {});
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
