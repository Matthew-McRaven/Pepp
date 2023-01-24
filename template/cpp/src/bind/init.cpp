#include <napi.h>
#include <node_api.h>

#include "./sample.hpp"

void dummy() {}
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "Test"), bind::TestWrapper::GetClass(env));

  return exports;
}

NODE_API_MODULE(addon, Init);
