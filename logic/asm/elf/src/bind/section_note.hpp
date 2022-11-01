//
// Created by gpu on 10/27/22.
//
#pragma once

#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>

namespace bind {

class NoteAccessor : public Napi::ObjectWrap<NoteAccessor> {
public:
  // Takes 2 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is a Section.
  NoteAccessor(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value get_index(const Napi::CallbackInfo &info);

  Napi::Value get_note_count(const Napi::CallbackInfo &info);
  Napi::Value get_note(const Napi::CallbackInfo &info);
  Napi::Value add_note(const Napi::CallbackInfo &info);
private:
  ELFIO::elfio *elf;
  std::shared_ptr <ELFIO::note_section_accessor> notes;
  ELFIO::section *section;

};
}
