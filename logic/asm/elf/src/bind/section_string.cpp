//
// Created by gpu on 10/27/22.
//

#include "./section.hpp"
#include "./section_string.hpp"
#include "./utils.hpp"
#include "./top_level.hpp"

bind::StringAccessor::StringAccessor(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<StringAccessor>(info) {
  bind::detail::count_args(info, 2, 2);
  this->elf =
      bind::detail::parse_arg_wrapped<bind::Elf>(info, 0, "elf file")->get_elfio();
  this->section = bind::detail::parse_arg_wrapped<bind::Section>(info, 1, "elf section")->get_raw_section();
  this->strs = std::make_shared<ELFIO::string_section_accessor>(section);
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
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);

  auto string = bind::detail::parse_arg_string(info, 0, "string");
  return Napi::BigInt::New(env, static_cast<uint64_t>(strs->add_string(string)));
}

Napi::Function bind::StringAccessor::GetClass(Napi::Env env) {
  return bind::StringAccessor::DefineClass(env, "StringAccessor", {
      StringAccessor::InstanceMethod("getIndex", &StringAccessor::get_index),
      StringAccessor::InstanceMethod("getString", &StringAccessor::get_string),
      StringAccessor::InstanceMethod("addString", &StringAccessor::add_string),
  });
}
