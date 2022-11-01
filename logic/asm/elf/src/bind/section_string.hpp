//
// Created by gpu on 10/27/22.
//
#pragma once

#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>

namespace bind {

class StringAccessor : public Napi::ObjectWrap<StringAccessor> {
public:
  // Takes 2 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is a Section
  StringAccessor(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value get_index(const Napi::CallbackInfo &info);

  // bigint=>string|undefined
  Napi::Value get_string(const Napi::CallbackInfo &info);
  // string=>bigint
  Napi::Value add_string(const Napi::CallbackInfo &info);

private:
  ELFIO::elfio *elf;
  std::shared_ptr <ELFIO::string_section_accessor> strs;
  ELFIO::section *section;
  Napi::Object strcache;
};
};
