//
// Created by gpu on 10/27/22.
//
#pragma once

#include <napi.h>
#include <node_api.h>
#include <fmt/core.h>

namespace bind::detail {

void count_args(const Napi::CallbackInfo &info, std::size_t min, std::size_t max);

uint64_t parse_arg_bigint(const Napi::CallbackInfo &info, std::size_t position, std::string type_name);
uint64_t parse_arg_number(const Napi::CallbackInfo &info, std::size_t position, std::string type_name);
std::string parse_arg_string(const Napi::CallbackInfo &info, std::size_t position, std::string type_name);
Napi::TypedArray parse_arg_uint8array(const Napi::CallbackInfo &info, std::size_t position, std::string type_name);
Napi::Object parse_arg_object(const Napi::CallbackInfo &info, std::size_t position, std::string type_name);

template<typename T>
T *parse_arg_external(const Napi::CallbackInfo &info, std::size_t position, std::string type_name);
template<typename T>
T *parse_arg_wrapped(const Napi::CallbackInfo &info, std::size_t position, std::string type_name);

template<typename T>
T *parse_arg_external(const Napi::CallbackInfo &info, std::size_t position, std::string type_name) {
  auto env = info.Env();
  if (!info[position].IsExternal()) {
    auto error_message = fmt::format("Argument {} must be a external {}", position, type_name);
    Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
  }
  return info[position].As<Napi::External<T>>().Data();
}

template<typename T>
T *parse_arg_wrapped(const Napi::CallbackInfo &info, std::size_t position, std::string type_name) {
  auto env = info.Env();
  if (!info[position].IsObject()) {
    auto error_message = fmt::format("Argument {} must be a wrapped native {}", position, type_name);
    Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
  }
  return Napi::ObjectWrap<T>::Unwrap(info[position].ToObject());
}

}