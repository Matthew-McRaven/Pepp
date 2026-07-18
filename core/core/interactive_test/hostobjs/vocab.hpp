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
 * obj.free      ( idx[obj] -- ), free the object at index, removing it from the heap
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
  // Return a pointer to the underlying object data.
  // It is used to perform a reverse lookup from the data pointer to the value wrapper.
  virtual void *data() const = 0;
};

/*
 * Implemented in config.cpp
 * Register the following words:
 * cfg         ( -- addr), variable holding the configuration object being worked on by cfg.w* words.
 * cfg.parent  ( -- addr), variable holding the parent configuration object of the current configuration object.
 *                         Only intended for use when creating nested JSON objects (e.g., SimpleBus mappings).
 * cfg.alloc   (  -- idx[cfg]), allocate a new configuration object and push its index onto the stack
 *                 The type of the config is determined by the string argument (ptr to null terminated str).
 *                 next words is the name of the config
 * cfg.set     ( -- ) read next two words. first word is name, second is value.
 * cfg.append  ( -- ) read the next word (key). If cfg.parent[key] is an array, append cfg as the last entry. Reset cfg
 *                    to cfg.parent
 * cfg.insert  ( -- ) read the next word (key). Set cfg.parent[key] to cfg. Reset cfg to cfg.parent
 * cfg.print   ( -- ), reads next word as a field name. print the associate field on cfg.
 * cfg.has     ( -- 0|1), read next word, treat as field name. pushes 1 if cfg contains that field, 0 if not.
 * cfg.get     ( -- buf len), read next word, treat as field name. copies the string value of that field name into
                 buf, and pushes len.
 * cfg.dump    ( -- ) print all fields and values of the configuration object to output

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
  void *data() const override;

private:
  std::shared_ptr<nlohmann::json> _fields = nullptr;
};

/*
 * Implemented in system.cpp
 * sys           (-- addr), variable holding the system object being worked on by sys.w* words.
 * dev.parent    ( -- addr ), variable for holding the parent device
 * sys.alloc     ( -- ) allocate a new system object and push its index onto the stack. reads from cfg,
 *                   writes to sys, dev.parent
 * sys.init      ( -- ) recursively initialize() the devices of the system
 * sys.devcount  ( -- u16), push the number of devices in the system onto the stack
 * sys.device    ( u16 -- u16), given a device ID, push the heap index of that device onto the stack.
 * sys.json      ( -- ), print the system's JSON to output
 */
void register_system_words(Interpreter *interp);
class DeviceValue : public AValue {
  // AValue interface
public:
  DeviceValue(Device *dev);
  std::string type_name() const override;
  Type type_code() const override;
  std::string describe() const override;
  void *data() const override;
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
 * dev.alloc      ( -- ), read system, parent, and cfg before allocating a device in dev
 * dev.basename   ( -- buf len) get dev's basename into buffer on stack
 * dev.fullname   ( -- buf len) get dev's fullname into buffer on stack
 * dev.type       ( -- u16) push the type of the dev onto the stack
 * dev.id         ( -- u16) push the id of the dev onto the stack
 */
void register_device_words(Interpreter *interp);

/*
 * Implemented in target.cpp
 * tgt.read16     ( src len dst -- ), src is number in targets addr space, dst is a location in vm
 * tgt.write16    ( src len dst -- ), src is an address in vm addr space, dst is in targets
 *
 * All reads/writes are performed with BufferInternalAccess. Addresses are sign-extended to 32 bits for the target.
 */
void register_target_words(Interpreter *interp);
// Register all of the words defined in this directory.
void register_devicemgmt_words(Interpreter *interp);