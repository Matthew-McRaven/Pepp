#include <boost/algorithm/string.hpp>
#include <iostream>

#include "isa_bind.hpp"
#include "isa/pep10.hpp"

Napi::Value bind::unary_mnemonic_to_opcode(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 string argument").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "Expected first argument to be a string").ThrowAsJavaScriptException();
    return env.Null();
  }

  auto as_string = info[0].ToString().Utf8Value();
  auto isa_def = isa::pep10::isa_definition::get_definition();
  auto it = std::find_if(isa_def.isa.cbegin(), isa_def.isa.cend(), [as_string](auto entry) {
    return boost::iequals(isa::pep10::as_string(entry.first), as_string);
  });
  if (it == isa_def.isa.cend())
    return env.Undefined();
  else if (!isa::pep10::is_opcode_unary(it->first))
    return env.Undefined();

  auto retVal = Napi::Uint8Array::New(env, 1);
  retVal[0] = isa::pep10::opcode(it->first);
  return retVal;
}

Napi::Value bind::nonunary_mnemonic_to_opcode(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 2) {
    Napi::TypeError::New(env, "Expected 2 string arguments").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "Expected first argument to be a string").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[1].IsString()) {
    Napi::TypeError::New(env, "Expected second argument to be a string").ThrowAsJavaScriptException();
    return env.Null();
  }

  auto mnemonic_as_string = info[0].ToString().Utf8Value();
  auto isa_def = isa::pep10::isa_definition::get_definition();
  auto it = std::find_if(isa_def.isa.cbegin(), isa_def.isa.cend(), [mnemonic_as_string](auto entry) {
    return boost::iequals(isa::pep10::as_string(entry.first), mnemonic_as_string);
  });
  if (it == isa_def.isa.cend())
    return env.Undefined();
  else if (isa::pep10::is_opcode_unary(it->first))
    return env.Undefined();
  auto addr_mode_as_string = info[1].ToString().Utf8Value();
  auto addrMode = isa::pep10::parse_addr_mode(addr_mode_as_string);
  if (addrMode == isa::pep10::addressing_mode::INVALID)
    return env.Undefined();

  auto retVal = Napi::Uint8Array::New(env, 1);
  retVal[0] = isa::pep10::opcode(it->first, addrMode);
  return retVal;
}