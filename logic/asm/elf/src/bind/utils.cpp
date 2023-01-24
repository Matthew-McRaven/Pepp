//
// Created by gpu on 10/27/22.
//

#include <fmt/core.h>

#include "utils.hpp"

void bind::detail::count_args(const Napi::CallbackInfo &info, std::size_t min, std::size_t max) {
  auto env = info.Env();
  if (info.Length() < min || info.Length() > max) {
    if (min == max)
      Napi::TypeError::New(env, fmt::format("Expected {} argument(s)", min)).ThrowAsJavaScriptException();
    else
      Napi::TypeError::New(env, fmt::format("Expected [{},{}] argument(s)", min, max)).ThrowAsJavaScriptException();
  }
}

uint64_t bind::detail::parse_arg_bigint(const Napi::CallbackInfo &info, std::size_t position, std::string type_name) {
  auto env = info.Env();
  if (!info[position].IsBigInt()) {
    auto error_message = fmt::format("Argument {} must be a {}", position, type_name);
    Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
  } else {
    bool lossless;
    auto u64 = info[position].As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, fmt::format("Argument {} must fit in 64 bits", position)).ThrowAsJavaScriptException();
    return u64;
  }
  throw new std::logic_error("Unreachable parse error");
}

uint64_t bind::detail::parse_arg_number(const Napi::CallbackInfo &info, std::size_t position, std::string type_name) {
  auto env = info.Env();
  if (!info[position].IsNumber()) {
    auto error_message = fmt::format("Argument {} must be a {}", position, type_name);
    Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
  } else {
    return info[position].ToNumber().Int64Value();
  }
  throw new std::logic_error("Unreachable parse error");
}

std::string bind::detail::parse_arg_string(const Napi::CallbackInfo &info,
                                           std::size_t position,
                                           std::string type_name) {
  auto env = info.Env();
  if (!info[position].IsString()) {
    auto error_message = fmt::format("Argument {} must be a {}", position, type_name);
    Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
  }
  return info[position].ToString().Utf8Value();
}

Napi::TypedArray bind::detail::parse_arg_uint8array(const Napi::CallbackInfo &info,
                                                    std::size_t position,
                                                    std::string type_name) {
  auto env = info.Env();
  if (!info[position].IsTypedArray()) {
    auto error_message = fmt::format("Argument {} must be a {}", position, type_name);
    Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
  }
  return info[position].As<Napi::TypedArray>();
}
Napi::Object bind::detail::parse_arg_object(const Napi::CallbackInfo &info,
                                            std::size_t position,
                                            std::string type_name) {
  auto env = info.Env();
  if (!info[position].IsObject()) {
    auto error_message = fmt::format("Argument {} must be a {}", position, type_name);
    Napi::TypeError::New(env, error_message).ThrowAsJavaScriptException();
  }
  return info[position].ToObject();
}
