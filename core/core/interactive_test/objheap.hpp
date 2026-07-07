#pragma once

#include <string>
#include "core/interactive_test/interp.hpp"
#include "core/sim/memory/ram/dense.hpp"
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
  std::shared_ptr<System> value() const { return _value; }

private:
  std::shared_ptr<System> _value;
};

// ( -- idx[sys])
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

void system_devcount(Interpreter *interp);
// ( idx[sys] -- u16)
inline static const NativeOpcode SystemDevCount{
    .stack_delta = 0,
    .name = "system.devcount",
    .h = system_devcount,
};

class Configuration : public AValue {
public:
  virtual ~Configuration() override = default;
  virtual std::string get_field(std::string_view name) const = 0;
  virtual void set_field(std::string_view name, std::string_view value) = 0;
};

class DenseConfigValue : public Configuration {
public:
  ~DenseConfigValue() override = default;
  std::string type_name() const override { return "Dense::Config"; }
  std::string describe() const override { return "<values aqui>"; }
  std::string get_field(std::string_view name) const override;
  void set_field(std::string_view name, std::string_view value) override;
  static std::shared_ptr<DenseConfigValue> make();
  const Dense::Configuration &value() const { return _value; }

private:
  Dense::Configuration _value;
};

class DeviceValue : public AValue {};

class DenseDeviceValue : public DeviceValue {

public:
  ~DenseDeviceValue() override = default;
  std::string type_name() const override { return "Dense::Device"; }
  std::string describe() const override { return "<dense device object>"; }
  static std::shared_ptr<DenseDeviceValue> make(std::shared_ptr<SystemValue>, std::shared_ptr<DenseConfigValue> config);

private:
  // Lifetime of Dense* is tied implicitly to system, so don't let system be garbage collected.
  std::shared_ptr<System> _sys;
  // Non-owning ptr
  Dense *_value;
};

// ( -- idx[cfg])
void make_dense_config(Interpreter *interp);
inline static const NativeOpcode MakeDenseConfig{
    .stack_delta = 2,
    .name = "dense.config.create",
    .h = make_dense_config,
};

// ( idx[system] idx[config] -- idx[device])
void make_dense_device(Interpreter *interp);
inline static const NativeOpcode MakeDenseDevice{
    .stack_delta = -2,
    .name = "dense.device.create",
    .h = make_dense_device,
};
void register_native_heap_fns(Interpreter *interp);
