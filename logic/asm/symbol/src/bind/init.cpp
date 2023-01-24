#include <napi.h>
#include <node_api.h>

#include "./symbol.hpp"
#include "./table.hpp"

#ifndef ADDR_TYPE
#define ADDR_TYPE uint16_t
#define ADDR_SUFFIX "u16"
#endif

void dummy() {}
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  // Napi::String value = Napi::String::New(env, "Value");
  // exports.Set(value, Value<ADDR_TYPE>::GetClass(env, ADDR_SUFFIX));
  Napi::String symbol = Napi::String::New(env, "Symbol");
  exports.Set(symbol, Symbol<ADDR_TYPE>::GetClass(env, ADDR_SUFFIX));
  Napi::String leaf_table = Napi::String::New(env, "LeafTable");
  exports.Set(leaf_table, LeafTable<ADDR_TYPE>::GetClass(env, ADDR_SUFFIX));
  Napi::String branch_table = Napi::String::New(env, "BranchTable");
  exports.Set(branch_table, BranchTable<ADDR_TYPE>::GetClass(env, ADDR_SUFFIX));

  return exports;
}

NODE_API_MODULE(addon, Init);
