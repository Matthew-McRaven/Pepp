#include <napi.h>
#include <node_api.h>

#include "./isa/isa_bind.hpp"

#ifndef ADDR_TYPE
#define ADDR_TYPE uint16_t
#define ADDR_SUFFIX "u16"
#endif

void init() {}
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  auto isa = Napi::Object::New(env);
  isa.Set(Napi::String::New(env, "unaryMnemonicToOpcode"), Napi::Function::New<bind::unary_mnemonic_to_opcode>(env));
  isa.Set(Napi::String::New(env, "nonunaryMnemonicToOpcode"),
          Napi::Function::New<bind::nonunary_mnemonic_to_opcode>(env));
  exports.Set(Napi::String::New(env, "isa"), isa);
  return exports;
}

NODE_API_MODULE(addon, Init);
