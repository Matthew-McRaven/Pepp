#include "section.hpp"
#include "utils.hpp"

bind::Section::Section(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<Section>(info) {
  bind::detail::count_args(info, 2, 2);
  this->elf =
      *bind::detail::parse_arg_external<std::shared_ptr<ELFIO::elfio>>(info, 0, "shared pointer to an elf file");
  this->section = bind::detail::parse_arg_external<ELFIO::section>(info, 1, "raw pointer to an elf segment");
}

Napi::Value bind::Section::get_index(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_index()));
}

Napi::Value bind::Section::get_name(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::String::New(info.Env(), section->get_name());
}

Napi::Value bind::Section::set_name(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  section->set_name(bind::detail::parse_arg_string(info, 0, "string"));
  return info.Env().Null();
}

Napi::Value bind::Section::get_type(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_type()));
}

Napi::Value bind::Section::set_type(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  section->set_type(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Section::get_flags(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_flags()));
}

Napi::Value bind::Section::set_flags(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  section->set_flags(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Section::get_info(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_info()));
}

Napi::Value bind::Section::set_info(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  section->set_info(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Section::get_link(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_link()));
}

Napi::Value bind::Section::set_link(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  section->set_link(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Section::get_align(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_addr_align()));
}

Napi::Value bind::Section::set_align(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  section->set_addr_align(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Section::get_address(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_address()));
}

Napi::Value bind::Section::set_address(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  section->set_address(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Section::get_size(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_size()));
}

Napi::Value bind::Section::set_data(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  auto array = bind::detail::parse_arg_uint8array(info, 0, "Uint8Array");
  auto buffer = array.ArrayBuffer();
  section->set_data((const char *) buffer.Data(), buffer.ByteLength());
  return info.Env().Null();
}

Napi::Value bind::Section::append_data(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  auto array = bind::detail::parse_arg_uint8array(info, 0, "Uint8Array");
  auto buffer = array.ArrayBuffer();
  section->append_data((const char *) buffer.Data(), buffer.ByteLength());
  return info.Env().Null();
}

Napi::Function bind::Section::GetClass(Napi::Env env) {
  return bind::Section::DefineClass(env, "Section", {
      Section::InstanceMethod("getIndex", &Section::get_index),
      Section::InstanceMethod("getName", &Section::get_name),
      Section::InstanceMethod("setName", &Section::set_name),
      Section::InstanceMethod("getType", &Section::get_type),
      Section::InstanceMethod("setType", &Section::set_type),
      Section::InstanceMethod("getFlags", &Section::get_flags),
      Section::InstanceMethod("setFlags", &Section::set_flags),
      Section::InstanceMethod("getInfo", &Section::get_info),
      Section::InstanceMethod("setInfo", &Section::set_info),
      Section::InstanceMethod("getLink", &Section::get_link),
      Section::InstanceMethod("setLink", &Section::set_link),
      Section::InstanceMethod("getAlign", &Section::get_align),
      Section::InstanceMethod("setAlign", &Section::set_align),
      Section::InstanceMethod("getAddress", &Section::get_address),
      Section::InstanceMethod("setAddress", &Section::set_address),
      Section::InstanceMethod("getSize", &Section::get_size),
      Section::InstanceMethod("setData", &Section::set_data),
      Section::InstanceMethod("appendData", &Section::append_data),
  });
}