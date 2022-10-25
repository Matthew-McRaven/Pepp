#pragma once

#include <map>
#include <napi.h>
#include <node_api.h>
#include <elfio/elfio.hpp>
namespace bind {

class ELFWriter : public Napi::ObjectWrap<ELFWriter> {
public:
  // Takes 1 arg as number. must be 32 or 64.
  explicit ELFWriter(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  // Metadata writers
  // address: UByte4 as bigint
  Napi::Value write_entry_point(const Napi::CallbackInfo &info);
  // e_type: UByte2 as bigint
  Napi::Value write_e_type(const Napi::CallbackInfo &info);
  // e_machine: UByte2 as bigint
  Napi::Value write_e_machine(const Napi::CallbackInfo &info);
  // ????
  Napi::Value write_os_abi(const Napi::CallbackInfo &info);

  // strtab Helpers
  // If name is not present in the string table section, add the string and return it's string index.
  // If it does exist, just return the string index.
  // section_name: string, name: string
  Napi::Value write_string(const Napi::CallbackInfo &info);

  // Code / Data writers
  // section name as string, section flags as SectionHeaderFlags32, bytes as Uint8Array.
  // Will also be used by TS to create additional debug sections.
  // Returns UByte2 of section's index.
  Napi::Value write_section_bytes(const Napi::CallbackInfo &info);

  // Symbol / String  writers
  // section name as string, symbols as ElfSymbol32[]
  Napi::Value write_symbols(const Napi::CallbackInfo &info);

  // Relocation writers
  // string section_name (must be .REL or .RELA), relocation objects as (ELFRel32|ELFRelA32)[]
  Napi::Value write_relocations(const Napi::CallbackInfo &info);

  // file name as string
  // Returns boolean describing if the write was successful.
  Napi::Value dump_to_file(const Napi::CallbackInfo &info);
private:
  uint8_t bitness;
  std::shared_ptr<ELFIO::elfio> elf;
  std::map<std::string /*strtab section name*/, std::map<std::string /*string contents*/, uint64_t/*string index*/>>
      string_cache;
};
} //end namespace elf