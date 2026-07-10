#include "./vocab.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "fmt/format.h"

AValue::~AValue() = default;

void native_value_typename(Interpreter *interp) {
  u16 idx = interp->pop_psp<u16>();
  auto obj = interp->get_object(idx);
  interp->append_output(fmt::format("{}\n", obj ? obj->type_name() : "null"));
}
inline static const NativeOpcode ValueTypeName{
    .stack_delta = -2,
    .name = "obj.typename",
    .h = native_value_typename,
};

void native_value_type(Interpreter *interp) {
  u16 idx = interp->pop_psp<u16>();
  auto obj = interp->get_object(idx);
  interp->push_psp((u16)(obj ? (u16)obj->type_code() : (u16)AValue::Type::Undefined));
}
inline static const NativeOpcode ValueType{
    .stack_delta = 0,
    .name = "obj.type",
    .h = native_value_type,
};

void native_value_describe(Interpreter *interp) {
  u16 idx = interp->pop_psp<u16>();
  auto obj = interp->get_object(idx);
  interp->append_output(fmt::format("{}\n", obj ? obj->describe() : "null"));
}
inline static const NativeOpcode ValueDescribe{
    .stack_delta = -2,
    .name = "obj.describe",
    .h = native_value_describe,
};

void dump_objects(Interpreter *interp) {
  auto heap = interp->get_object_heap();
  for (const auto &[id, obj] : heap)
    interp->append_output(fmt::format("[{:04x}](*{}):  {}\n", id, obj->type_name(), obj->describe()));
}
inline static const NativeOpcode DumpObjects{
    .stack_delta = 0,
    .name = "dumpobjs",
    .h = dump_objects,
};

void register_value_words(Interpreter *p) {
  dict_insert_native(p, ValueTypeName, {});
  dict_insert_native(p, ValueType, {});
  dict_insert_native(p, ValueDescribe, {});
  dict_insert_native(p, DumpObjects, {});
  const u16 arg1_spad = p->cb.here;
  p->cb.here += 32;
  NativeOpcode PushA1 = {
      .stack_delta = 2,
      .name = "t1",
      .h = [arg1_spad](Interpreter *i) { push_constant(i, arg1_spad); },
  };
  dict_insert_native(p, PushA1, {});
  p->run_on(": word!t1 word t1 cmove ;");

  const u16 arg2_spad = p->cb.here;
  p->cb.here += 32;
  NativeOpcode PushA2 = {
      .stack_delta = 2,
      .name = "t2",
      .h = [arg2_spad](Interpreter *i) { push_constant(i, arg2_spad); },
  };
  dict_insert_native(p, PushA2, {});
  p->run_on(": word!t2 word t2 cmove ;");
}