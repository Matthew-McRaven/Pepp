#pragma once

#include <napi.h>
#include <node_api.h>

#include "symbol/entry.hpp"

template<typename addr_size_t>
class Symbol : public Napi::ObjectWrap<Symbol<addr_size_t>> {
public:
  Symbol(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env, std::string suffix);
  Napi::Value name(const Napi::CallbackInfo &info);
  Napi::Value definition_state(const Napi::CallbackInfo &info);
  Napi::Value binding(const Napi::CallbackInfo &info);
  Napi::Value value(const Napi::CallbackInfo &info);
  Napi::Value type(const Napi::CallbackInfo &info);
  Napi::Value size(const Napi::CallbackInfo &info);
  Napi::Value relocatable(const Napi::CallbackInfo &info);
  // If two symbol values share a symbolIndex, then they point to the same underlying symbol.
  Napi::Value symbol_index(const Napi::CallbackInfo &info);

  Napi::Value is_const(const Napi::CallbackInfo &);
  Napi::Value set_const(const Napi::CallbackInfo &info);

  Napi::Value is_addr(const Napi::CallbackInfo &);
  // Valid strings for arg 3 are "code" and "object"
  Napi::Value set_addr(const Napi::CallbackInfo &info);
  Napi::Value offset(const Napi::CallbackInfo &info);
  Napi::Value base(const Napi::CallbackInfo &info);

  Napi::Value is_symptr(const Napi::CallbackInfo &info);
  Napi::Value set_symptr(const Napi::CallbackInfo &info);

  Napi::Value is_empty(const Napi::CallbackInfo &info);
  Napi::Value set_empty(const Napi::CallbackInfo &info);
  Napi::Value is_deleted(const Napi::CallbackInfo &info);
  Napi::Value set_deleted(const Napi::CallbackInfo &info);

  std::shared_ptr<symbol::entry<addr_size_t>> raw_symbol() { return _symbol; };
private:
  std::shared_ptr<symbol::entry<addr_size_t>> _symbol;
};

#include "./symbol.tpp"