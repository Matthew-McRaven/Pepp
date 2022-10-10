#include <napi.h>
#include <node_api.h>

#ifndef ADDR_TYPE
#define ADDR_TYPE uint16_t
#define ADDR_SUFFIX "u16"
#endif

void dummy() {}
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return exports;
}

NODE_API_MODULE(addon, Init);
