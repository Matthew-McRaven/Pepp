#pragma once

#include <nlohmann/json_fwd.hpp>
#include "../interp.hpp"
#include "core/ds/string_compare.hpp"
#include "core/integers.h"
#include "core/sim/api/device.hpp"

class Interpreter;

/*
 * Implemented in avalue.cpp
 * Register the following words:
 * obj.typename  ( idx[obj] -- ), print the type name of the object at idx to output
 * obj.type      ( idx[obj] -- u16), push the type code of the object at index onto the stack
 * obj.describe  ( idx[obj] -- ), print the description of the object at idx to output
 * t1            ( -- addr), push the address of a scratch buffer onto the stack
 * word!t1       ( -- ), execute word and copy its buffer to t1
 * t2            ( -- addr), push the address of a scratch buffer onto the stack
 * word!t2       ( -- ), execute word and copy its buffer to t2
 * dumpobjs      ( -- ), print the contents of the object table to output
 */
void register_value_words(Interpreter *interp);
class AValue {
public:
  enum class Type : u16 {
    Undefined = 0,
    InteractiveConfig = 1,
    Device = 2,
  };
  virtual ~AValue() = 0;
  virtual std::string type_name() const = 0;
  virtual Type type_code() const = 0;
  virtual std::string describe() const = 0;
};

/*
 * Implemented in config.cpp
 * Register the following words:
 * cfg         ( -- addr), variable holding the configuration object being worked on by cfg.w* words.
 * cfg.alloc   (name  -- idx[cfg]), allocate a new configuration object and push its index onto the stack
 *                 The type of the config is determined by the string argument (ptr to null terminated str).
 * cfg.walloc  ( -- idx[cfg]), reads the next word and call alloc with it. Sets cfg.
 * cfg.set     (idx[cfg] name value -- ), treating name and value as ptrs to null terminated strings,
 *                 call set_field on the configuration object
 * cfg.wset    ( -- ), reads the next two words and call set with them
 * cfg.print   (idx[cfg] name -- ), treating name as a cstr, get the associated field and print its value as a string.
 * cfg.wprint  ( -- ), reads the next word and call print with it
 * cfg.has     (idx[cfg] name -- u16), treating name as a cstr, push 1 if the field exists, 0 otherwise.
 * cfg.whas    (-- u16), reads the next word and call has with it
 * cfg.get     (idx[cfg] name buf -- len ok), treating name as a cstr, copy the field value as a string into buf, and
 *                 push the len written. If ok is false, the field does not exist
 * cfg.wget    ( -- buf len ok), reads the next word and call get_str with it
 * cfg.dump    (idx[cfg] -- ), print all fields and values of the configuration object to output
 * cfg.wdump   ( -- )
 */
void register_config_words(Interpreter *interp);
class InteractiveConfiguration : public AValue {
public:
  InteractiveConfiguration();
  virtual ~InteractiveConfiguration() override = default;
  std::string get_field(std::string_view name) const;
  void set_field(std::string_view name, std::string_view value);
  bool has_field(std::string_view name) const;
  static std::shared_ptr<InteractiveConfiguration> make();

  // AValue interface
  std::string type_name() const override;
  Type type_code() const override;
  std::string describe() const override;
  const nlohmann::json &fields() const;
  nlohmann::json &fields();

private:
  std::shared_ptr<nlohmann::json> _fields = nullptr;
};

/*
 * Implemented in system.cpp
 * sys           (-- addr), variable holding the system object being worked on by sys.w* words.
 * dev.parent    ( -- addr ), variable for holding the parent device
 * sys.alloc     (idx[cfx]  -- idx[sys]), allocate a new system object and push its index onto the stack
 * sys.walloc    ( -- idx[sys]). Sets both sys and parent.
 * sys.devcount  (idx[sys] -- u16), push the number of devices in the system onto the stack
 * sys.wdevcount ( -- u16)
 */
void register_system_words(Interpreter *interp);
class DeviceValue : public AValue {
  // AValue interface
public:
  DeviceValue(Device *dev);
  std::string type_name() const override;
  Type type_code() const override;
  std::string describe() const override;
  static std::shared_ptr<DeviceValue> make(Device *);
  Device *dev;
};
class SystemValue : public DeviceValue {
public:
  SystemValue(std::unique_ptr<System> sys);
  static std::shared_ptr<DeviceValue> make(std::unique_ptr<System> sys);
  std::unique_ptr<System> sys;
  u16 device_count() const;
};

/*
 * Implemented in dev.cpp
 * dev            ( -- addr ), variable for holding the current device
 * dev.alloc      (idx[sys] idx[par] idx[cfg] -- idx[dev]), allocate a new device and push its index onto the stack
 * dev.walloc     ( -- ), Read system, parent, and cfg before allocating a device.
 * dev.basename   (idx[dev] buf -- len) write the basename of the device to buf, pushing len onto stack
 * dev.wbasename  ( -- buf len) get device basename into buffer on stack
 * dev.fullname   (idx[dev] buf -- len) write the fullname of the device to buf, pushing len onto stack
 * dev.wfullname  ( -- buf len) get device fullname into buffer on stack
 * dev.type       (idx[dev] -- u16) push the type of the device onto the stack
 * dev.wtype      ( -- u16) push the type of the device onto the stack
 * dev.id         (idx[dev] -- u16) push the id of the device onto the stack
 * dev.wid        ( -- u16) push the id of the device onto the stack
 */
void register_device_words(Interpreter *interp);
// Register all of the words defined in this directory.
void register_devicemgmt_words(Interpreter *interp);