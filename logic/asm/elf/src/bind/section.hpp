#pragma once

#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>

namespace bind {
class Section : public Napi::ObjectWrap<Section> {
public:
  // Takes 2 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is ELFIO::section*.
  Section(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value get_index(const Napi::CallbackInfo &info);

  Napi::Value get_name(const Napi::CallbackInfo &info);
  Napi::Value set_name(const Napi::CallbackInfo &info);

  Napi::Value get_type(const Napi::CallbackInfo &info);
  Napi::Value set_type(const Napi::CallbackInfo &info);

  Napi::Value get_flags(const Napi::CallbackInfo &info);
  Napi::Value set_flags(const Napi::CallbackInfo &info);

  Napi::Value get_info(const Napi::CallbackInfo &info);
  Napi::Value set_info(const Napi::CallbackInfo &info);

  Napi::Value get_link(const Napi::CallbackInfo &info);
  Napi::Value set_link(const Napi::CallbackInfo &info);

  Napi::Value get_align(const Napi::CallbackInfo &info);
  Napi::Value set_align(const Napi::CallbackInfo &info);

  Napi::Value get_address(const Napi::CallbackInfo &info);
  Napi::Value set_address(const Napi::CallbackInfo &info);

  Napi::Value get_size(const Napi::CallbackInfo &info);
  Napi::Value set_data(const Napi::CallbackInfo &info);
  Napi::Value append_data(const Napi::CallbackInfo &info);

  // Used by segment, accessors
  ELFIO::section *get_raw_Section() { return section; }
private:
  std::shared_ptr<ELFIO::elfio> elf;
  ELFIO::section *section;
};
}