#include "./vocab.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "fmt/format.h"

std::string InteractiveConfiguration::get_field(std::string_view name) const {
  if (auto v = _fields.find(name); v == _fields.end()) return "";
  else return v->second;
}

void InteractiveConfiguration::set_field(std::string_view name, std::string_view value) {
  _fields[std::string(name)] = std::string(value);
}

bool InteractiveConfiguration::has_field(std::string_view name) const { return _fields.find(name) != _fields.end(); }

std::shared_ptr<InteractiveConfiguration> InteractiveConfiguration::make() {
  return std::make_shared<InteractiveConfiguration>();
}

std::string InteractiveConfiguration::type_name() const { return "Configuration"; }

std::string InteractiveConfiguration::describe() const {
  return fmt::format("<Configuration with {} fields>", _fields.size());
}

AValue::Type InteractiveConfiguration::type_code() const { return AValue::Type::InteractiveConfig; }

void native_cfg_alloc(Interpreter *interp) {
  auto addr = interp->pop_psp<u16>();
  auto len = strlen_helper(interp, addr);
  std::string_view type_str{(const char *)interp->memory.data() + addr, len};
  // TODO: prefill fields (like compatible) based on the type_str/
  auto cfg = InteractiveConfiguration::make();
  interp->push_psp((u16)interp->allocate_object(cfg));
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
      interp->push_psp((u16)1); // ok
    } else {
      interp->push_psp((u16)0); // len
      interp->push_psp((u16)0); // ok
    }
  }
}
inline static const NativeOpcode CfgGet{
    .stack_delta = -2,
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
  else {
    for (const auto &[name, value] : *casted) {
      interp->append_output(fmt::format("  {}: {}\n", name, value));
    }
  }
}
inline static const NativeOpcode CfgDump{
    .stack_delta = -2,
    .name = "cfg.dump",
    .h = native_cfg_dump,
};

void register_config_words(Interpreter *p) {
  dict_insert_native(p, CfgAlloc, {});
  p->run_on(": cfg.walloc word cfg.alloc ;");
  dict_insert_native(p, CfgSet, {});
  p->run_on(": cfg.wset word!t1 word!t2 t1 t2 cfg.set ;");
  dict_insert_native(p, CfgPrint, {});
  p->run_on(": cfg.wprint word!t1 t1 cfg.print ;");
  dict_insert_native(p, CfgHas, {});
  p->run_on(": cfg.whas word!t1 t1 cfg.has ;");
  dict_insert_native(p, CfgGet, {});
  // Bury t2 under cfg[idx] before building the rest of the stack for get
  // (e.g., name buf). When get consumes cfg-name-buf, t2 is on the stack
  // which combines with returned vals of get (len ok) to form (t2/buf len ok)
  p->run_on(": cfg.wget t2 swap word!t1 t1 t2 cfg.get ;");
  dict_insert_native(p, CfgDump, {});
}