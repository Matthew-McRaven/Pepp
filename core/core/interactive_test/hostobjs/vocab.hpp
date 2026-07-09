#pragma once

#include "../interp.hpp"
#include "core/integers.h"
class Interpreter;

/*
 * Implemented in avalue.cpp
 * Register the following words:
 * obj.typename  ( idx[obj] -- ), print the type name of the object at idx to output
 * obj.type      ( idx[obj] -- u16), push the type code of the object at index onto the stack
 * obj.describe  ( idx[obj] -- ), print the description of the object at idx to output
 */
void register_value_words(Interpreter *interp);
class AValue {
public:
  enum class Type : u16 {
    Undefined = 0,
    Config_System = 1,
    Device_System = 2,
  };
  virtual ~AValue() = 0;
  virtual std::string type_name() const = 0;
  virtual Type type_code() const = 0;
  virtual std::string describe() const = 0;
};

// Register all of the words defined in this directory.
void register_devicemgmt_words(Interpreter *interp);

/*
 * Implemented in acfg.cpp
 * Register the following words:
 * cfg.alloc   (addr len -- idx[cfg]), allocate a new configuration object and push its index onto the stack
 *                 The type of the config is determined by the string argument.
 * cfg.walloc  ( -- idx[cfg]), reads the next word and call alloc with it
 * cfg.set     (idx[cfg] name value -- ), treating name and value as ptrs to null terminated strings,
 *                 call set_field on the configuration object
 * cfg.wset    (idx[cfg] -- ), reads the next two words and call set with them
 * cfg.print   (idx[cfg] name -- ), treating name as a cstr, get the associated field and print its value as a string.
 * cfg.wprint  (idx[cfg] -- ), reads the next word and call print with it
 * cfg.has     (idx[cfg] name -- u16), treating name as a cstr, push 1 if the field exists, 0 otherwise.
 * cfg.whas    (idx[cfg] -- u16), reads the next word and call has with it
 * cfg.type    (idx[cfg] name -- u16), push the type code of the configuration field onto the stack
 * cfg.wtype   (idx[cfg] -- u16), reads the next word and call type with it
 * cfg.get_str (idx[cfg] name buf -- len ok), treating name as a cstr, copy the field value as a string into buf, and
 *                 push the len written. If ok is false, the field does not exist
 * cfg.wget_str(idx[cfg] -- buf len ok), reads the next word and call get_str with it
 * cfg.get_int (idx[cfg] name -- i16 ok), treating name as a cstr, push the field value as an integer onto the stack.
 *                 if ok is 0, the field could not be converted to an int.
 * cfg.wget_int(idx[cfg] -- i16 ok), reads the next word and call get_int with it
 */
void register_config_words(Interpreter *interp);

class Configuration : public AValue {
public:
  enum class Type : u16 {
    undefined = 0,
    i16 = 1,
  };
  virtual ~Configuration() override = default;
  virtual std::string get_field(std::string_view name) const = 0;
  virtual void set_field(std::string_view name, std::string_view value) = 0;
  virtual bool has_field(std::string_view name) const { return false; }
  virtual Type field_type(std::string_view name) const { return Type::undefined; }
};