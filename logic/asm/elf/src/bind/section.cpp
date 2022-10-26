#include "section.hpp"
bind::ELFSection::ELFSection(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ELFSection>(info) {
  auto env = info.Env();
  if (info.Length() != 2) {
    Napi::TypeError::New(env, "Expected 2 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsExternal()) {
    Napi::TypeError::New(env, "First argument must be a shared pointer to an elf file").ThrowAsJavaScriptException();
  } else if (!info[1].IsExternal()) {
    Napi::TypeError::New(env, "Second argument must be a raw pointer to an elf section").ThrowAsJavaScriptException();
  }
  this->elf = *info[0].As<Napi::External<std::shared_ptr<ELFIO::elfio>>>().Data();
  this->section = info[1].As<Napi::External<ELFIO::section>>().Data();
}

Napi::Value bind::ELFSection::get_address(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
  }

  return Napi::BigInt::New(env, static_cast<uint64_t>(section->get_address()));
}

Napi::Value bind::ELFSection::get_size(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
  }

  return Napi::BigInt::New(env, static_cast<uint64_t>(section->get_size()));
}

Napi::Function bind::ELFSection::GetClass(Napi::Env env) {
  return bind::ELFSection::DefineClass(env, "ELFSection", {
      ELFSection::InstanceMethod("getAddress", &ELFSection::get_address),
      ELFSection::InstanceMethod("getSize", &ELFSection::get_size),
  });
}