#include <napi.h>
#include <node_api.h>

#include "./section.hpp"
#include "./section_note.hpp"
#include "./section_relocation.hpp"
#include "./section_string.hpp"
#include "./section_symbol.hpp"

#include "segment.hpp"

#include "./top_level.hpp"

void dummy() {}
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "Section"), bind::Section::GetClass(env));
  exports.Set(Napi::String::New(env, "NoteAccessor"), bind::NoteAccessor::GetClass(env));
  exports.Set(Napi::String::New(env, "RelAccessor"), bind::RelAccessor::GetClass(env));
  exports.Set(Napi::String::New(env, "RelAAccessor"), bind::RelAAccessor::GetClass(env));
  exports.Set(Napi::String::New(env, "StringAccessor"), bind::StringAccessor::GetClass(env));
  exports.Set(Napi::String::New(env, "SymbolAccessor"), bind::SymbolAccessor::GetClass(env));
  exports.Set(Napi::String::New(env, "Segment"), bind::Segment::GetClass(env));
  exports.Set(Napi::String::New(env, "Elf"), bind::Elf::GetClass(env));

  exports.Set(Napi::String::New(env, "saveElfToFile"), Napi::Function::New<bind::save_file>(env));
  exports.Set(Napi::String::New(env, "loadElfFromFile"), Napi::Function::New<bind::load_file>(env));
  exports.Set(Napi::String::New(env, "saveElfToBuffer"), Napi::Function::New<bind::save_buffer>(env));
  //exports.Set(Napi::String::New(env, "loadElfFromBuffer"), Napi::Function::New<bind::load_buffer>(env));

  return exports;
}

NODE_API_MODULE(addon, Init);
