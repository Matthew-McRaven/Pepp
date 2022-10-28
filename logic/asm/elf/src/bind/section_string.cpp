//
// Created by gpu on 10/27/22.
//

#include "./section_string.hpp"
#include "./utils.hpp"

bind::StringAccessor::StringAccessor(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<StringAccessor>(info) {
  bind::detail::count_args(info, 3, 3);
  this->elf =
      *bind::detail::parse_arg_external<std::shared_ptr<ELFIO::elfio>>(info, 0, "shared pointer to an elf file");
  this->section = bind::detail::parse_arg_external<ELFIO::section>(info, 1, "raw pointer to an elf segment");
  this->strs = std::make_shared<ELFIO::string_section_accessor>(section);
  this->strcache = bind::detail::parse_arg_object(info, 2, "StringCache");
}

Napi::Value bind::StringAccessor::get_index(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_index()));
}

Napi::Value bind::StringAccessor::get_string(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto index = bind::detail::parse_arg_bigint(info, 0, "bigint");

  auto str = this->strs->get_string(index);
  if (str == nullptr)
    return env.Undefined();
  else
    return Napi::String::New(env, str);
}

Napi::Value bind::StringAccessor::add_string(const Napi::CallbackInfo &info) {
  static const auto error_has = "Argument 3 must have a 'has(string, string)=>bigint|undefined'";
  static const auto error_insert = "Argument 3 must have a 'insert(string, string, bigint)=>bigint'";

  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);

  auto string = bind::detail::parse_arg_string(info, 0, "string");
  // Convert section name to Napi::String to do cache lookup
  auto section_str = Napi::String::New(env, section->get_name());

  // Check if the symcache has a "has" function. If so, call it as has(string,string)=>bigint|undefined.
  if (!this->strcache.Has("has"))
    Napi::TypeError::New(env, error_has).ThrowAsJavaScriptException();
  auto has = this->strcache.Get("has");
  if (!has.IsFunction())
    Napi::TypeError::New(env, error_has).ThrowAsJavaScriptException();
  auto result = has.As<Napi::Function>().Call({section_str, info[0]});

  // If "has" returns undefined, then the string isn't in the cache. We need to add the string to the section+cache.
  if (result.IsUndefined()) {
    auto index_as_bigint = Napi::BigInt::New(env, static_cast<uint64_t>(strs->add_string(string)));

    // Check if the symcache has an "insert" function. If so, call it as insert(string,string,bigint)=>bigint.
    if (!this->strcache.Has("insert"))
      Napi::TypeError::New(env, error_insert).ThrowAsJavaScriptException();
    auto insert = this->strcache.Get("insert");
    if (!insert.IsFunction())
      Napi::TypeError::New(env, error_insert).ThrowAsJavaScriptException();
    return has.As<Napi::Function>().Call({section_str, info[0], index_as_bigint});
  } else
    return result;
}

Napi::Function bind::StringAccessor::GetClass(Napi::Env env) {
  return bind::StringAccessor::DefineClass(env, "StringAccessor", {
      StringAccessor::InstanceMethod("getIndex", &StringAccessor::get_index),
      StringAccessor::InstanceMethod("getString", &StringAccessor::get_string),
      StringAccessor::InstanceMethod("addString", &StringAccessor::add_string),
  });
}
