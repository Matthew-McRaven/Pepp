//
// Created by gpu on 10/27/22.
//
#pragma once

#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>

namespace bind {

class SymbolAccessor : public Napi::ObjectWrap<SymbolAccessor> {
public:
  // Takes 3 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is a Section, 3rd is a Section.
  SymbolAccessor(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value get_index(const Napi::CallbackInfo &info);

  // void => bigint
  Napi::Value get_symbol_count(const Napi::CallbackInfo &info);
  // bigint => Symbol | undefined
  Napi::Value get_symbol(const Napi::CallbackInfo &info);
  // Symbol => bigint
  Napi::Value add_symbol(const Napi::CallbackInfo &info);

  // void => void
  Napi::Value update_info(const Napi::CallbackInfo &info);

private:
  ELFIO::elfio *elf;
  std::shared_ptr <ELFIO::string_section_accessor> strs;
  std::shared_ptr <ELFIO::symbol_section_accessor> syms;
  ELFIO::section *strSec, *symSec;
  // Always at least 1 because of symbol 0, the undefined symbol
  uint64_t symbol_count = 1;
};
};
