//
// Created by gpu on 10/27/22.
//

#include "./section.hpp"
#include "./top_level.hpp"
#include "./section_note.hpp"
#include "./utils.hpp"

bind::NoteAccessor::NoteAccessor(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NoteAccessor>(info) {
  bind::detail::count_args(info, 2, 2);
  this->elf =
      bind::detail::parse_arg_wrapped<bind::Elf>(info, 0, "elf file")->get_elfio();
  this->section = bind::detail::parse_arg_wrapped<bind::Section>(info, 1, "elf section")->get_raw_section();
  this->notes = std::make_shared<ELFIO::note_section_accessor>(*elf, section);
}

Napi::Value bind::NoteAccessor::get_index(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(section->get_index()));
}

Napi::Value bind::NoteAccessor::get_note_count(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(notes->get_notes_num()));
}

Napi::Value bind::NoteAccessor::get_note(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto note_index = bind::detail::parse_arg_bigint(info, 0, "bigint");
  ELFIO::Elf_Word desc_size;
  ELFIO::Elf_Word type;
  std::string name;
  char *desc;

  auto success = notes->get_note(note_index, type, name, desc, desc_size);
  if (!success)
    return env.Undefined();
  else {
    auto object = Napi::Object::New(env);
    auto arrayBuffer = Napi::ArrayBuffer::New(env, desc, desc_size);
    object.Set("type", Napi::BigInt::New(env, static_cast<uint64_t>(type)));
    object.Set("name", Napi::String::New(env, name));
    object.Set("desc", Napi::Uint8Array::New(env, desc_size, arrayBuffer, 0));
    return object;
  }
}

Napi::Value bind::NoteAccessor::add_note(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto object = bind::detail::parse_arg_object(info, 0, "Object");
  if (!object.Has("type") || !object.Get("type").IsBigInt())
    Napi::TypeError::New(env, "Argument 1 must have property 'type' as bigint").ThrowAsJavaScriptException();
  else if (!object.Has("name") || !object.Get("name").IsString())
    Napi::TypeError::New(env, "Argument 1 must have property 'name' as string").ThrowAsJavaScriptException();
  else if (!object.Has("desc") || !object.Get("desc").IsTypedArray())
    Napi::TypeError::New(env, "Argument 1 must have property 'desc' as Uint8Array").ThrowAsJavaScriptException();

  bool lossless;

  uint64_t type = object.Get("type").As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless)
    Napi::TypeError::New(env, "type must fit in 64 bits").ThrowAsJavaScriptException();
  std::string name = object.Get("name").ToString().Utf8Value();
  auto desc_buffer = object.Get("desc").As<Napi::TypedArray>().ArrayBuffer();
  if (!lossless)
    Napi::TypeError::New(env, "size must fit in 64 bits").ThrowAsJavaScriptException();
  notes->add_note(type, name, (const char *) desc_buffer.Data(), desc_buffer.ByteLength());
  return env.Null();
}

Napi::Function bind::NoteAccessor::GetClass(Napi::Env env) {
  return bind::NoteAccessor::DefineClass(env, "NoteAccessor", {
      NoteAccessor::InstanceMethod("getIndex", &NoteAccessor::get_index),
      NoteAccessor::InstanceMethod("getNoteCount", &NoteAccessor::get_note_count),
      NoteAccessor::InstanceMethod("getNote", &NoteAccessor::get_note),
      NoteAccessor::InstanceMethod("addNote", &NoteAccessor::add_note),

  });
}
