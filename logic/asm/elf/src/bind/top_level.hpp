#pragma once

#include <map>
#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>
namespace bind {

class Elf : public Napi::ObjectWrap<Elf> {
public:
  // Takes 1 args. 1st arg is object StringCache.
  explicit Elf(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value init(const Napi::CallbackInfo &info);
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

  Napi::Value get_default_entry_size(const Napi::CallbackInfo &info);

  Napi::Value add_section(const Napi::CallbackInfo &info);
  Napi::Value get_section(const Napi::CallbackInfo &info);
  Napi::Value section_count(const Napi::CallbackInfo &info);

  Napi::Value add_segment(const Napi::CallbackInfo &info);
  Napi::Value get_segment(const Napi::CallbackInfo &info);
  Napi::Value segment_count(const Napi::CallbackInfo &info);

  ELFIO::elfio *get_elfio() { return &*elf; }
private:
  void validate_elf_ptr(const Napi::CallbackInfo &info);
  std::shared_ptr<ELFIO::elfio> elf;
  std::map<uint64_t, ELFIO::section *> sec_map;
  std::map<uint64_t, ELFIO::segment *> seg_map;
};

Napi::Value save_file(const Napi::CallbackInfo &info);
Napi::Value load_file(const Napi::CallbackInfo &info);
Napi::Value save_buffer(const Napi::CallbackInfo &info);
//Napi::Value load_buffer(const Napi::CallbackInfo &info);
} //end namespace elf
