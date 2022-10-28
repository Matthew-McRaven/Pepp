#pragma once

#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>

namespace bind {
class Segment : public Napi::ObjectWrap<Segment> {
public:
  // Takes 2 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is ELFIO::segment*.
  Segment(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value get_index(const Napi::CallbackInfo &info);

  Napi::Value get_type(const Napi::CallbackInfo &info);
  Napi::Value set_type(const Napi::CallbackInfo &info);

  Napi::Value get_align(const Napi::CallbackInfo &info);
  Napi::Value set_align(const Napi::CallbackInfo &info);

  Napi::Value get_vaddress(const Napi::CallbackInfo &info);
  Napi::Value set_vaddress(const Napi::CallbackInfo &info);

  Napi::Value get_paddress(const Napi::CallbackInfo &info);
  Napi::Value set_paddress(const Napi::CallbackInfo &info);

  Napi::Value get_memory_size(const Napi::CallbackInfo &info);
  Napi::Value set_memory_size(const Napi::CallbackInfo &info);

  Napi::Value get_file_size(const Napi::CallbackInfo &info);

  Napi::Value get_flags(const Napi::CallbackInfo &info);
  Napi::Value set_flags(const Napi::CallbackInfo &info);

  // Takes in external section.
  Napi::Value get_section_count(const Napi::CallbackInfo &info);
  Napi::Value get_section_index_at(const Napi::CallbackInfo &info);
  Napi::Value add_section(const Napi::CallbackInfo &info);
  Napi::Value add_section_index(const Napi::CallbackInfo &info);
  Napi::Value recompute_file_size(const Napi::CallbackInfo &info);

private:
  void recompute_file_size_impl();
  std::shared_ptr<ELFIO::elfio> elf;
  ELFIO::segment *segment;
};
}