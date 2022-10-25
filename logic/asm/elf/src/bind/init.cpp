#include <napi.h>
#include <node_api.h>

#include "writer.hpp"
#ifndef ADDR_TYPE
#define ADDR_TYPE uint16_t
#define ADDR_SUFFIX "u16"
#endif

void dummy() {}
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "ELFWriter"), bind::ELFWriter::GetClass(env));
  return exports;
}

NODE_API_MODULE(addon, Init);
