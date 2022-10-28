//
// Created by gpu on 10/27/22.
//

#include "./section_symbol.hpp"
#include "./utils.hpp"
#include <variant>
#include <fmt/core.h>

bind::SymbolAccessor::SymbolAccessor(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SymbolAccessor>(info) {
  bind::detail::count_args(info, 4, 4);
  this->elf =
      *bind::detail::parse_arg_external<std::shared_ptr<ELFIO::elfio>>(info, 0, "shared pointer to an elf file");
  this->strSec = bind::detail::parse_arg_external<ELFIO::section>(info, 1, "raw pointer to an elf section");
  this->strcache = bind::detail::parse_arg_object(info, 2, "StringCache");
  this->symSec = bind::detail::parse_arg_external<ELFIO::section>(info, 3, "raw pointer to an elf section");
  this->strs = std::make_shared<ELFIO::string_section_accessor>(strSec);
  this->syms = std::make_shared<ELFIO::symbol_section_accessor>(*elf, symSec);
}

Napi::Value bind::SymbolAccessor::get_index(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(symSec->get_index()));
}

Napi::Value bind::SymbolAccessor::get_symbol_count(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(syms->get_symbols_num()));
}

Napi::Value bind::SymbolAccessor::get_symbol(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  std::variant<uint64_t, std::string> arg;
  if (info[0].IsString()) {
    arg = bind::detail::parse_arg_string(info, 0, "string");
  } else if (info[0].IsBigInt()) {
    arg = bind::detail::parse_arg_bigint(info, 0, "bigint");
  }

  std::string name;
  ELFIO::Elf64_Addr value;
  ELFIO::Elf_Xword size;
  unsigned char bind, type;
  ELFIO::Elf_Half shndx;
  unsigned char other;

  if (holds_alternative<std::string>(arg)) {
    bool found = false;
    for (auto idx = 0; idx < syms->get_symbols_num(); idx++) {
      auto innerFind = syms->get_symbol(idx, name, value, size, bind, type, shndx, other);
      if (innerFind && name == std::get<std::string>(arg)) {
        found = true;
        break;
      }
    }
    if (!found)
      return env.Undefined();
  } else {
    if (!syms->get_symbol(std::get<uint64_t>(arg), name, value, size, bind, type, shndx, other))
      return env.Undefined();
  }
  auto object = Napi::Object::New(env);
  object.Set("name", Napi::String::New(env, name));
  object.Set("value", Napi::BigInt::New(env, static_cast<uint64_t>(value)));
  object.Set("size", Napi::BigInt::New(env, static_cast<uint64_t>(size)));
  object.Set("binding", Napi::BigInt::New(env, static_cast<uint64_t>(bind)));
  object.Set("type", Napi::BigInt::New(env, static_cast<uint64_t>(type)));
  object.Set("other", Napi::BigInt::New(env, static_cast<uint64_t>(other)));
  object.Set("shndx", Napi::BigInt::New(env, static_cast<uint64_t>(shndx)));
  return object;
}

Napi::Value bind::SymbolAccessor::add_symbol(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto object = bind::detail::parse_arg_object(info, 0, "Object");

  bool lossless;
  auto keys = std::vector({"value", "size", "binding", "type", "other", "shndx"});
  auto v = std::vector<uint64_t>(keys.size());
  int index = 0;

  // Ensure that all keys are present as bigints, and parse into values array 'v'.
  for (const auto &key : keys) {
    if (!object.Has(key) || !object.Get(key).IsBigInt()) {
      auto error_message = fmt::format("Argument 1 must have property '{}' as bigint", key);
      Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
    }
    v[index++] = object.Get(key).As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, fmt::format("{} must fit in 64 bits", key)).ThrowAsJavaScriptException();
  }

  uint64_t str = 0;
  if (object.Get("name").IsBigInt()) {
    str = object.Get("name").As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "name must fit in 64 bits").ThrowAsJavaScriptException();
  }
  uint64_t symIdx = syms->add_symbol(str, v[0], v[1], v[2], v[3], v[4], v[5]);
  return Napi::BigInt::New(env, symIdx);
}

Napi::BigInt bind::SymbolAccessor::add_string(const Napi::CallbackInfo &info, std::string value) {
  static const auto error_has = "symcache must have a 'has(string, string)=>bigint|undefined'";
  static const auto error_insert = "symcache must have a 'insert(string, string, bigint)=>bigint'";

  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);

  // Convert std::strings to JS strings
  auto value_str = Napi::String::New(env, value);
  auto section_str = Napi::String::New(env, strSec->get_name());

  // Check if the symcache has a "has" function. If so, call it as has(string,string)=>bigint|undefined.
  if (!this->strcache.Has("has"))
    Napi::TypeError::New(env, error_has).ThrowAsJavaScriptException();
  auto has = this->strcache.Get("has");
  if (!has.IsFunction())
    Napi::TypeError::New(env, error_has).ThrowAsJavaScriptException();
  auto result = has.As<Napi::Function>().Call({section_str, value_str});

  // If "has" returns undefined, then the string isn't in the cache. We need to add the string to the section+cache.
  if (result.IsUndefined()) {
    auto index_as_bigint = Napi::BigInt::New(env, static_cast<uint64_t>(strs->add_string(value)));

    // Check if the symcache has an "insert" function. If so, call it as insert(string,string,bigint)=>bigint.
    if (!this->strcache.Has("insert"))
      Napi::TypeError::New(env, error_insert).ThrowAsJavaScriptException();
    auto insert = this->strcache.Get("insert");
    if (!insert.IsFunction())
      Napi::TypeError::New(env, error_insert).ThrowAsJavaScriptException();
    return has.As<Napi::Function>().Call({section_str, info[0], index_as_bigint}).As<Napi::BigInt>();
  } else
    return result.As<Napi::BigInt>();
}

Napi::Function bind::SymbolAccessor::GetClass(Napi::Env env) {
  return bind::SymbolAccessor::DefineClass(env, "SymbolAccessor", {
      SymbolAccessor::InstanceMethod("getIndex", &SymbolAccessor::get_index),
      SymbolAccessor::InstanceMethod("getSymbolCount", &SymbolAccessor::get_symbol_count),
      SymbolAccessor::InstanceMethod("getSymbol", &SymbolAccessor::get_symbol),
      SymbolAccessor::InstanceMethod("addSymbol", &SymbolAccessor::add_symbol),
  });
}
