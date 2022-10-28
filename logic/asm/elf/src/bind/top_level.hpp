#pragma once

#include <map>
#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>
namespace bind {

class Elf : public Napi::ObjectWrap<Elf> {
public:
  // Takes 2 args. 1st arg must be 32 or 64, 2nd arg is object StringCache.
  explicit Elf(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value validate(const Napi::CallbackInfo &info);

  Napi::Value get_class(const Napi::CallbackInfo &info);

  Napi::Value get_version(const Napi::CallbackInfo &info);

  Napi::Value get_os_abi(const Napi::CallbackInfo &info);
  Napi::Value set_os_abi(const Napi::CallbackInfo &info);
  Napi::Value get_abi_version(const Napi::CallbackInfo &info);
  Napi::Value set_abi_version(const Napi::CallbackInfo &info);

  Napi::Value get_type(const Napi::CallbackInfo &info);
  Napi::Value set_type(const Napi::CallbackInfo &info);

  Napi::Value get_machine(const Napi::CallbackInfo &info);
  Napi::Value set_machine(const Napi::CallbackInfo &info);

  Napi::Value get_flags(const Napi::CallbackInfo &info);
  Napi::Value set_flags(const Napi::CallbackInfo &info);

  Napi::Value get_entry(const Napi::CallbackInfo &info);
  Napi::Value set_entry(const Napi::CallbackInfo &info);

  Napi::Value add_section(const Napi::CallbackInfo &info);
  Napi::Value get_section(const Napi::CallbackInfo &info);

  Napi::Value add_segment(const Napi::CallbackInfo &info);
  Napi::Value get_segment(const Napi::CallbackInfo &info);

  Napi::Value get_string_cache(const Napi::CallbackInfo &info);

  ELFIO::elfio *get_elfio() { return &*elf; }
private:
  std::shared_ptr<ELFIO::elfio> elf;
  Napi::Object str_cache;
  std::map<uint64_t, Napi::Object> sec_map, seg_map;
};

Napi::Value save_file(const Napi::CallbackInfo &info);
Napi::Value load_file(const Napi::CallbackInfo &info);
Napi::Value save_buffer(const Napi::CallbackInfo &info);
Napi::Value load_buffer(const Napi::CallbackInfo &info);
} //end namespace elf