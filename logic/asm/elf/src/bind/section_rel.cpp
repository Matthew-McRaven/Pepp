//
// Created by gpu on 10/27/22.
//

#include "./section_relocation.hpp"
#include "./utils.hpp"

bind::RelAccessor::RelAccessor(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<RelAccessor>(info) {
  bind::detail::count_args(info, 2, 2);
  this->elf =
      *bind::detail::parse_arg_external<std::shared_ptr<ELFIO::elfio>>(info, 0, "shared pointer to an elf file");
  this->section = bind::detail::parse_arg_external<ELFIO::section>(info, 1, "raw pointer to an elf segment");
  this->rels = std::make_shared<ELFIO::relocation_section_accessor>(*elf.get(), section);
}

Napi::Value bind::RelAccessor::get_index(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_index()));
}

Napi::Value bind::RelAccessor::get_entry_count(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(rels->get_entries_num()));
}

Napi::Value bind::RelAccessor::get_rel_entry(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto index = bind::detail::parse_arg_bigint(info, 0, "bigint");

  ELFIO::Elf64_Addr offset;
  ELFIO::Elf_Word symbol;
  ELFIO::Elf_Word type;
  ELFIO::Elf_Sxword _unused_;

  auto success = rels->get_entry(index, offset, symbol, type, _unused_);
  if (!success)
    return env.Undefined();
  else {
    auto object = Napi::Object::New(env);
    object.Set("offset", Napi::BigInt::New(env, offset));
    object.Set("sym", Napi::BigInt::New(env, static_cast<uint64_t>(symbol)));
    object.Set("type", Napi::BigInt::New(env, static_cast<uint64_t>(type)));
    return object;
  }
}

Napi::Value bind::RelAccessor::add_rel_entry(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto object = bind::detail::parse_arg_object(info, 0, "Object");
  if (!object.Has("offset") || !object.Get("offset").IsBigInt())
    Napi::TypeError::New(env, "Argument 1 must have property 'offset' as bigint").ThrowAsJavaScriptException();
  else if (!object.Has("sym") || !object.Get("sym").IsBigInt())
    Napi::TypeError::New(env, "Argument 1 must have property 'sym' as bigint").ThrowAsJavaScriptException();
  else if (!object.Has("type") || !object.Get("type").IsBigInt())
    Napi::TypeError::New(env, "Argument 1 must have property 'desc' as bigint").ThrowAsJavaScriptException();

  bool lossless;

  uint64_t offset = object.Get("offset").As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless)
    Napi::TypeError::New(env, "offset must fit in 64 bits").ThrowAsJavaScriptException();

  uint64_t sym = object.Get("sym").As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless)
    Napi::TypeError::New(env, "sym must fit in 64 bits").ThrowAsJavaScriptException();

  uint64_t type = object.Get("type").As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless)
    Napi::TypeError::New(env, "type must fit in 64 bits").ThrowAsJavaScriptException();

  rels->add_entry(offset, static_cast<ELFIO::Elf_Word>(sym), static_cast<unsigned char>(type));
  return env.Null();
}

Napi::Function bind::RelAccessor::GetClass(Napi::Env env) {
  return bind::RelAccessor::DefineClass(env, "RelAccessor", {
      RelAccessor::InstanceMethod("getIndex", &RelAccessor::get_index),
      RelAccessor::InstanceMethod("getEntryCount", &RelAccessor::get_entry_count),
      RelAccessor::InstanceMethod("getRelEntry", &RelAccessor::get_rel_entry),
      RelAccessor::InstanceMethod("addRelEntry", &RelAccessor::add_rel_entry),

  });
}