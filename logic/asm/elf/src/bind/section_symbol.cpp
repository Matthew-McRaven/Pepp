//
// Created by gpu on 10/27/22.
//

#include "./section.hpp"
#include "./section_symbol.hpp"
#include "./utils.hpp"
#include <variant>
#include <fmt/core.h>

#include "./top_level.hpp"

bind::SymbolAccessor::SymbolAccessor(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SymbolAccessor>(info) {
  bind::detail::count_args(info, 3, 3);
  this->elf =
      bind::detail::parse_arg_wrapped<bind::Elf>(info, 0, "elf file")->get_elfio();
  this->strSec = bind::detail::parse_arg_wrapped<bind::Section>(info, 1, "elf section")->get_raw_section();
  this->symSec = bind::detail::parse_arg_wrapped<bind::Section>(info, 2, "elf section")->get_raw_section();
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
  std::variant <uint64_t, std::string> arg;
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
      auto error_message = fmt::format("Argument 0 must have property '{}' as bigint", key);
      Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
    }
    v[index++] = object.Get(key).As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, fmt::format("{} must fit in 64 bits", key)).ThrowAsJavaScriptException();
  }

  uint64_t name = 0;
  auto name_key = object.Get("name");
  if (name_key.IsBigInt()) {
    bool lossless;
    name = name_key.As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, fmt::format("BigInt name must fit in 64 bits")).ThrowAsJavaScriptException();
  } else if (name_key.IsString()) {
    auto name_string = name_key.ToString().Utf8Value();
    name = strs->add_string(name_string);
  } else {
    // TODO: Throw error on unexpected type.
  }
  symbol_count++;
  uint64_t symIdx = syms->add_symbol(name, v[0], v[1], v[2], v[3], v[4], v[5]);
  return Napi::BigInt::New(env, symIdx);
}

Napi::Value bind::SymbolAccessor::update_info(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  symSec->set_info(symbol_count);
  return info.Env().Null();
}

Napi::Function bind::SymbolAccessor::GetClass(Napi::Env env) {
  return bind::SymbolAccessor::DefineClass(env, "SymbolAccessor", {
      SymbolAccessor::InstanceMethod("getIndex", &SymbolAccessor::get_index),
      SymbolAccessor::InstanceMethod("getSymbolCount", &SymbolAccessor::get_symbol_count),
      SymbolAccessor::InstanceMethod("getSymbol", &SymbolAccessor::get_symbol),
      SymbolAccessor::InstanceMethod("addSymbol", &SymbolAccessor::add_symbol),
      SymbolAccessor::InstanceMethod("updateInfo", &SymbolAccessor::update_info),
  });
}
