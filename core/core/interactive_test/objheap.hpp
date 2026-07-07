#pragma once

#include <string>
#include "core/interactive_test/interp.hpp"
#include "core/sim/system.hpp"

class Interpreter;
class AValue {
public:
  virtual ~AValue() = 0;
  virtual std::string type_name() const = 0;
  virtual std::string describe() const = 0;
};

void native_value_typename(Interpreter *interp);
inline static const NativeOpcode ValueTypeName{
    .stack_delta = -2,
    .name = "a.type-name",
    .h = native_value_typename,
};
void native_value_describe(Interpreter *interp);
inline static const NativeOpcode ValueDescribe{
    .stack_delta = -2,
    .name = "a.describe",
    .h = native_value_describe,
};

class SystemValue : public AValue {
public:
  SystemValue() = default;
  ~SystemValue() override = default;
  std::string type_name() const override { return "System"; }
  std::string describe() const override { return "System object"; }
  static std::shared_ptr<SystemValue> make();
  u16 devcount() const;

private:
  std::shared_ptr<System> _value;
};

void make_system(Interpreter *interp);
inline static const NativeOpcode MakeSystem{
    .stack_delta = 2,
    .name = "system.create",
    .h = make_system,
};

void dump_objects(Interpreter *interp);
inline static const NativeOpcode DumpObjects{
    .stack_delta = 0,
    .name = "heap.dumpobjs",
    .h = dump_objects,
};
void register_native_heap_fns(Interpreter *interp);
