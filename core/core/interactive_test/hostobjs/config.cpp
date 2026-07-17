#include <nlohmann/json.hpp>
#include "./vocab.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "core/sim/systemparser.hpp"
#include "fmt/format.h"

InteractiveConfiguration::InteractiveConfiguration() : _fields(std::make_shared<nlohmann::json>()) {}
const nlohmann::json &InteractiveConfiguration::fields() const { return *_fields; }

nlohmann::json &InteractiveConfiguration::fields() { return *_fields; }

void *InteractiveConfiguration::data() const {
  return (void *)_fields.get(); // Return the pointer to the underlying JSON object
}

std::string InteractiveConfiguration::get_field(std::string_view name) const {
  if (auto v = _fields->find(name); v == _fields->end()) return "";
  else return v->get<std::string>();
}

void InteractiveConfiguration::set_field(std::string_view name, std::string_view value) {
  (*_fields)[std::string(name)] = std::string(value);
}

bool InteractiveConfiguration::has_field(std::string_view name) const { return _fields->find(name) != _fields->end(); }

std::shared_ptr<InteractiveConfiguration> InteractiveConfiguration::make() {
  return std::make_shared<InteractiveConfiguration>();
}

std::string InteractiveConfiguration::type_name() const { return "Configuration"; }

std::string InteractiveConfiguration::describe() const {
  return fmt::format("<Configuration with {} fields>", _fields->size());
}

AValue::Type InteractiveConfiguration::type_code() const { return AValue::Type::InteractiveConfig; }

void native_cfg_alloc(Interpreter *interp) {
  auto addr = interp->pop_psp<u16>();
  auto len = strlen_helper(interp, addr);
  std::string_view type_str{(const char *)interp->memory.data() + addr, len};
  auto cfg = InteractiveConfiguration::make();
  prefill_keys(cfg->fields(), type_str);
  interp->push_psp<u16>(interp->allocate_object(cfg));
}
inline static const NativeOpcode CfgAlloc{
    .stack_delta = 0,
    .name = "cfg.alloc",
    .h = native_cfg_alloc,
};

void native_cfg_set(Interpreter *interp) {
  auto value_addr = interp->pop_psp<u16>();
  auto value_len = strlen_helper(interp, value_addr);

  auto name_addr = interp->pop_psp<u16>();
  auto name_len = strlen_helper(interp, name_addr);
  auto cfg_idx = interp->pop_psp<u16>();

  std::string_view name{(const char *)interp->memory.data() + name_addr, name_len};
  std::string_view value{(const char *)interp->memory.data() + value_addr, value_len};

  auto cfg = interp->get_object(cfg_idx);
  if (!cfg) throw std::runtime_error("Invalid configuration object index");
  if (auto casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
      cfg->type_code() != AValue::Type::InteractiveConfig || casted == nullptr)
    throw std::runtime_error("Object is not a configuration object");
  else casted->set_field(name, value);
}
inline static const NativeOpcode CfgSet{
    .stack_delta = -6,
    .name = "cfg.set",
    .h = native_cfg_set,
};

// ( parent_idx cfg_idx name-- ) Insert a JSON object into a parent configuration
void native_cfg_insert(Interpreter *interp) {
  auto name_addr = interp->pop_psp<u16>();
  auto name_len = strlen_helper(interp, name_addr);

  auto cfg_idx = interp->pop_psp<u16>();
  auto par_idx = interp->pop_psp<u16>();
  std::string_view name{(const char *)interp->memory.data() + name_addr, name_len};

  auto cfg = interp->get_object(cfg_idx);
  auto par = interp->get_object(par_idx);

  if (!cfg) throw std::runtime_error("Invalid configuration object index");
  else if (!par) throw std::runtime_error("Invalid parent configuration object index");
  else if (auto cfg_casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
           cfg->type_code() != AValue::Type::InteractiveConfig || cfg_casted == nullptr)
    throw std::runtime_error("Object is not a configuration object");
  else if (auto par_casted = std::dynamic_pointer_cast<InteractiveConfiguration>(par);
           par->type_code() != AValue::Type::InteractiveConfig || par_casted == nullptr)
    throw std::runtime_error("Parent object is not a configuration object");
  else par_casted->fields()[name] = cfg_casted->fields();
}
inline static const NativeOpcode CfgInsert{
    .stack_delta = -6,
    .name = "cfg.insert",
    .h = native_cfg_insert,
};

void native_cfg_append(Interpreter *interp) {
  auto name_addr = interp->pop_psp<u16>();
  auto name_len = strlen_helper(interp, name_addr);

  auto cfg_idx = interp->pop_psp<u16>();
  auto par_idx = interp->pop_psp<u16>();
  std::string_view name{(const char *)interp->memory.data() + name_addr, name_len};

  auto cfg = interp->get_object(cfg_idx);
  auto par = interp->get_object(par_idx);

  if (!cfg) throw std::runtime_error("Invalid configuration object index");
  else if (!par) throw std::runtime_error("Invalid parent configuration object index");
  else if (auto cfg_casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
           cfg->type_code() != AValue::Type::InteractiveConfig || cfg_casted == nullptr)
    throw std::runtime_error("Object is not a configuration object");
  else if (auto par_casted = std::dynamic_pointer_cast<InteractiveConfiguration>(par);
           par->type_code() != AValue::Type::InteractiveConfig || par_casted == nullptr)
    throw std::runtime_error("Parent object is not a configuration object");
  else {
    // If array, just append the child config
    if (auto json = par_casted->fields()[name]; json.is_array()) json.push_back(cfg_casted->fields());
    // If non-existent, must first create array, then insert.
    else if (json.is_null()) par_casted->fields()[name] = nlohmann::json::array({cfg_casted->fields()});
    else throw std::runtime_error("Field is not an array");
  }
}
inline static const NativeOpcode CfgAppend{
    .stack_delta = -6,
    .name = "cfg.append",
    .h = native_cfg_append,
};

void native_cfg_print(Interpreter *interp) {
  auto name_addr = interp->pop_psp<u16>();
  auto name_len = strlen_helper(interp, name_addr);
  auto cfg_idx = interp->pop_psp<u16>();

  std::string_view name{(const char *)interp->memory.data() + name_addr, name_len};

  auto cfg = interp->get_object(cfg_idx);
  if (!cfg) throw std::runtime_error("Invalid configuration object index");
  if (auto casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
      cfg->type_code() != AValue::Type::InteractiveConfig || casted == nullptr)
    throw std::runtime_error("Object is not a configuration object");
  else {
    auto value = casted->get_field(name);
    interp->append_output(fmt::format("{}\n", value));
  }
}
inline static const NativeOpcode CfgPrint{
    .stack_delta = -4,
    .name = "cfg.print",
    .h = native_cfg_print,
};

void native_cfg_has(Interpreter *interp) {
  auto name_addr = interp->pop_psp<u16>();
  auto name_len = strlen_helper(interp, name_addr);
  auto cfg_idx = interp->pop_psp<u16>();

  std::string_view name{(const char *)interp->memory.data() + name_addr, name_len};

  auto cfg = interp->get_object(cfg_idx);
  if (!cfg) throw std::runtime_error("Invalid configuration object index");
  if (auto casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
      cfg->type_code() != AValue::Type::InteractiveConfig || casted == nullptr)
    throw std::runtime_error("Object is not a configuration object");
  else {
    bool has_field = casted->has_field(name);
    interp->push_psp((u16)(has_field ? 1 : 0));
  }
}
inline static const NativeOpcode CfgHas{
    .stack_delta = -2,
    .name = "cfg.has",
    .h = native_cfg_has,
};
void native_cfg_get(Interpreter *interp) {
  auto buf_addr = interp->pop_psp<u16>();
  auto name_addr = interp->pop_psp<u16>();
  auto name_len = strlen_helper(interp, name_addr);
  auto cfg_idx = interp->pop_psp<u16>();

  std::string_view name{(const char *)interp->memory.data() + name_addr, name_len};

  auto cfg = interp->get_object(cfg_idx);
  if (!cfg) throw std::runtime_error("Invalid configuration object index");
  if (auto casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
      cfg->type_code() != AValue::Type::InteractiveConfig || casted == nullptr)
    throw std::runtime_error("Object is not a configuration object");
  else {
    bool has_field = casted->has_field(name);
    if (has_field) {
      auto value = casted->get_field(name);
      interp->write(buf_addr, std::span<const u8>((const u8 *)value.data(), value.size()));
      interp->write<u8>(0, buf_addr + value.size()); // null terminate
      interp->push_psp((u16)value.size());
    } else {
      interp->push_psp((u16)0); // len
    }
  }
}
inline static const NativeOpcode CfgGet{
    .stack_delta = -4,
    .name = "cfg.get",
    .h = native_cfg_get,
};

void native_cfg_dump(Interpreter *interp) {
  auto cfg_idx = interp->pop_psp<u16>();

  auto cfg = interp->get_object(cfg_idx);
  if (!cfg) throw std::runtime_error("Invalid configuration object index");
  if (auto casted = std::dynamic_pointer_cast<InteractiveConfiguration>(cfg);
      cfg->type_code() != AValue::Type::InteractiveConfig || casted == nullptr)
    throw std::runtime_error("Object is not a configuration object");
  else interp->append_output(casted->fields().dump() + "\n");
}
inline static const NativeOpcode CfgDump{
    .stack_delta = -2,
    .name = "cfg.dump",
    .h = native_cfg_dump,
};

void register_config_words(Interpreter *p) {

  p->run_on(" var cfg drop"); // create a var to hold the index of the WIP config. var returns the addr of the variable,
                              // which we don't need.
  p->run_on(" var cfg.parent drop");
  auto op_alloc = p->register_native(CfgAlloc);
  p->run_on(fmt::format(": cfg.alloc word!t1 t1 op 0x{:4x} cfg ! ; ", op_alloc));
  auto op_set = p->register_native(CfgSet);
  p->run_on(fmt::format(": cfg.set cfg @ word!t1 word!t2 t1 t2 op 0x{:4x} ;", op_set));
  auto op_insert = p->register_native(CfgInsert);
  p->run_on(fmt::format(": cfg.insert cfg.parent @ cfg @ word!t1 t1 op 0x{:4x} cfg.parent @ cfg ! ;", op_insert));
  auto op_append = p->register_native(CfgAppend);
  p->run_on(fmt::format(": cfg.append cfg.parent @ cfg @ word!t1 t1 op 0x{:4x} cfg.parent @ cfg ! ;", op_append));
  auto op_print = p->register_native(CfgPrint);
  p->run_on(fmt::format(": cfg.print cfg @ word!t1 t1 op 0x{:4x} ;", op_print));
  auto op_has = p->register_native(CfgHas);
  p->run_on(fmt::format(": cfg.has cfg @ word!t1 t1 op 0x{:4x} ;", op_has));
  auto op_get = p->register_native(CfgGet);
  // Bury t2 under cfg[idx] before building the rest of the stack for get
  // (e.g., name buf). When get consumes cfg-name-buf, t2 is on the stack
  // which combines with returned vals of get (len ok) to form (t2/buf len ok)
  p->run_on(fmt::format(": cfg.get t2 cfg @ word!t1 t1 t2 op 0x{:4x} ;", op_get));
  auto op_dump = p->register_native(CfgDump);
  p->run_on(fmt::format(": cfg.dump cfg @ op 0x{:4x} ;", op_dump));
}