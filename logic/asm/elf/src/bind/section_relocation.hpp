//
// Created by gpu on 10/27/22.
//
#pragma once

#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>

namespace bind {

class RelAccessor : public Napi::ObjectWrap<RelAccessor> {
public:
  // Takes 2 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is a Section.
  RelAccessor(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value get_index(const Napi::CallbackInfo &info);

  Napi::Value get_entry_count(const Napi::CallbackInfo &info);
  // Returns Rel or undefined
  Napi::Value get_rel_entry(const Napi::CallbackInfo &info);
  // Takes Rel as object, returns bool for success
  Napi::Value add_rel_entry(const Napi::CallbackInfo &info);

private:
  std::shared_ptr<ELFIO::elfio> elf;
  std::shared_ptr<ELFIO::relocation_section_accessor> rels;
  ELFIO::section *section;
};

class RelAAccessor : public Napi::ObjectWrap<RelAAccessor> {
public:
  // Takes 2 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is a Section.
  RelAAccessor(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value get_index(const Napi::CallbackInfo &info);

  Napi::Value get_entry_count(const Napi::CallbackInfo &info);
  // Returns RelA or undefined
  Napi::Value get_rela_entry(const Napi::CallbackInfo &info);
  // Takes RelA as object, returns bool for success
  Napi::Value add_rela_entry(const Napi::CallbackInfo &info);

private:
  std::shared_ptr<ELFIO::elfio> elf;
  std::shared_ptr<ELFIO::relocation_section_accessor> rels;
  ELFIO::section *section;
};
}