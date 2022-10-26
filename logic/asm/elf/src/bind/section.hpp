#pragma once

#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>

namespace bind {
class ELFSection : public Napi::ObjectWrap<ELFSection> {
public:
  // Takes 2 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is ELFIO::section*.
  ELFSection(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value get_address(const Napi::CallbackInfo &info);
  Napi::Value get_size(const Napi::CallbackInfo &info);
private:
  std::shared_ptr<ELFIO::elfio> elf;
  ELFIO::section *section;
};
}