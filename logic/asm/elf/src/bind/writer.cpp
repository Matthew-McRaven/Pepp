#include "writer.hpp"
#include <iostream>

#include "./section.hpp"

bind::ELFWriter::ELFWriter(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ELFWriter>(info), string_cache() {
  auto env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be a number").ThrowAsJavaScriptException();
  }
  bitness = info[0].As<Napi::Number>().Uint32Value();
  if (bitness != 32 && bitness != 64)
    Napi::TypeError::New(env, "Bitness must be 32 or 64").ThrowAsJavaScriptException();
  elf = std::make_shared<ELFIO::elfio>();
  elf->create(bitness == 32 ? ELFIO::ELFCLASS32 : ELFIO::ELFCLASS64, ELFIO::ELFDATA2LSB);
}

Napi::Value bind::ELFWriter::write_entry_point(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsBigInt()) {
    Napi::TypeError::New(env, "First argument must be a bigint").ThrowAsJavaScriptException();
  }

  bool lossless;
  auto entry_point = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless)
    Napi::TypeError::New(env, "First argument must fit in 64 bits").ThrowAsJavaScriptException();
  elf->set_entry(entry_point);
  return env.Null();
}

Napi::Value bind::ELFWriter::write_e_type(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsBigInt()) {
    Napi::TypeError::New(env, "First argument must be a bigint").ThrowAsJavaScriptException();
  }

  bool lossless;
  auto e_type = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless)
    Napi::TypeError::New(env, "First argument must fit in 64 bits").ThrowAsJavaScriptException();
  elf->set_type(e_type);
  return env.Null();
}

Napi::Value bind::ELFWriter::write_e_machine(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsBigInt()) {
    Napi::TypeError::New(env, "First argument must be a bigint").ThrowAsJavaScriptException();
  }

  bool lossless;
  auto e_machine = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless)
    Napi::TypeError::New(env, "First argument must fit in 64 bits").ThrowAsJavaScriptException();
  elf->set_machine(e_machine);
  return env.Null();
}

Napi::Value bind::ELFWriter::write_os_abi(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsBigInt()) {
    Napi::TypeError::New(env, "First argument must be a bigint").ThrowAsJavaScriptException();
  }

  bool lossless;
  auto abi = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless)
    Napi::TypeError::New(env, "First argument must fit in 64 bits").ThrowAsJavaScriptException();
  elf->set_os_abi(abi);
  return env.Null();
}

Napi::Value bind::ELFWriter::write_string(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 2) {
    Napi::TypeError::New(env, "Expected 2 arguments").ThrowAsJavaScriptException();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
  } else if (!info[1].IsString()) {
    Napi::TypeError::New(env, "Second argument must be a string").ThrowAsJavaScriptException();
  }
  auto section_name = info[0].ToString().Utf8Value();
  auto string_value = info[1].ToString().Utf8Value();
  return Napi::BigInt::New(env, static_cast<uint64_t>(write_string_impl(section_name, string_value)));
}

Napi::Value bind::ELFWriter::write_section_bytes(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 3) {
    Napi::TypeError::New(env, "Expected 3 arguments").ThrowAsJavaScriptException();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
  } else if (!info[1].IsObject()) {
    Napi::TypeError::New(env, "Second argument must be an object").ThrowAsJavaScriptException();
  } else if (!info[2].IsTypedArray()) {
    Napi::TypeError::New(env, "Third argument must be an Uint8Array").ThrowAsJavaScriptException();
  }

  auto section_name = info[0].ToString().Utf8Value();
  auto flags = info[1].ToObject();
  auto bytes = info[2].As<Napi::TypedArray>();

  auto section = elf->sections.add(section_name);
  // TODO: Set flags / alignment / type from flags object;
  section->set_type(ELFIO::SHT_PROGBITS);
  section->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_EXECINSTR | ELFIO::SHF_WRITE);
  section->set_addr_align(0x1);
  section->set_data((const char *) bytes.ArrayBuffer().Data(), bytes.ByteLength());
  return Napi::BigInt::New(env, static_cast<uint64_t>(section->get_index()));
}

Napi::Value bind::ELFWriter::write_symbols(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 3) {
    Napi::TypeError::New(env, "Expected 2 arguments").ThrowAsJavaScriptException();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
  } else if (!info[1].IsString()) {
    Napi::TypeError::New(env, "Second argument must be a string").ThrowAsJavaScriptException();
  } else if (!info[2].IsArray()) {
    Napi::TypeError::New(env, "Third argument must be an Array").ThrowAsJavaScriptException();
  }
  // Find or create the symbol table section
  auto strtab_name = info[0].ToString().Utf8Value();
  auto symtab_name = info[1].ToString().Utf8Value();

  auto [symtab_ptr, symtab_is_new] = this->get_or_create_section(symtab_name);
  ELFIO::symbol_section_accessor sym_ac(*elf, symtab_ptr);

  auto symbols = info[2].As<Napi::Array>();
  // If symbols are already in the table, extend the size of the table.
  // If no symbols, add +1 for the undefined symbol @ index 0.
  auto old = symtab_ptr->get_info();
  symtab_ptr->set_info((old == 0 ? 1 : old) + symbols.Length());

  bool lossless;
  uint64_t temp;
  for (int index = 0; index < symbols.Length(); index++) {
    auto symObject = symbols.Get(index).ToObject();

    ELFIO::Elf_Word name = write_string_impl(strtab_name, symObject.Get("st_name").ToString().Utf8Value());
    temp = symObject.Get("st_value").As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "st_value must be a bigint").ThrowAsJavaScriptException();
    ELFIO::Elf64_Addr value = temp;

    temp = symObject.Get("st_size").As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "st_size must be a bigint").ThrowAsJavaScriptException();
    ELFIO::Elf_Xword size = temp;

    temp = symObject.Get("st_info").As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "st_info must be a bigint").ThrowAsJavaScriptException();
    unsigned char info = temp;

    temp = symObject.Get("st_other").As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "st_other must be a bigint").ThrowAsJavaScriptException();
    unsigned char other = temp;

    temp = symObject.Get("st_shndx").As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "st_shndx must be a bigint").ThrowAsJavaScriptException();
    ELFIO::Elf_Half shndx = temp;
    sym_ac.add_symbol(name, value, size, info, other, shndx);
  }

  // Defer filling in these values, since strtab may not exist until after the filler loop.
  if (symtab_is_new) {
    symtab_ptr->set_type(ELFIO::SHT_SYMTAB);
    symtab_ptr->set_addr_align(0x2);
    symtab_ptr->set_entry_size(elf->get_default_entry_size(ELFIO::SHT_SYMTAB));
    symtab_ptr->set_link(std::get<0>(get_or_create_section(strtab_name))->get_index());
  }
  return env.Null();
}

Napi::Value bind::ELFWriter::write_relocations(const Napi::CallbackInfo &info) {
  // TODO: When we know how linker works
  auto env = info.Env();
  Napi::TypeError::New(env, "Unimplemented").ThrowAsJavaScriptException();
  return env.Null();
}

Napi::Value bind::ELFWriter::dump_to_file(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
  }
  auto file_name = info[0].ToString().Utf8Value();
  return Napi::Boolean::New(env, elf->save(file_name));
}

Napi::Value bind::ELFWriter::get_section(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
  }
  auto section_name = info[0].ToString().Utf8Value();
  auto [ptr, is_new] = this->get_or_create_section(section_name, false);
  if (ptr == nullptr)
    return env.Undefined();
  return ELFSection::GetClass(env).New({Napi::External<std::shared_ptr<ELFIO::elfio>>::New(env, &elf),
                                        Napi::External<ELFIO::section>::New(env, ptr)});
}

Napi::Function bind::ELFWriter::GetClass(Napi::Env env) {
  return bind::ELFWriter::DefineClass(env, "ELFWriter", {
      ELFWriter::InstanceMethod("writeEntryPoint", &ELFWriter::write_entry_point),
      ELFWriter::InstanceMethod("writeEType", &ELFWriter::write_e_type),
      ELFWriter::InstanceMethod("writeEMachine", &ELFWriter::write_e_machine),
      ELFWriter::InstanceMethod("writeOSABI", &ELFWriter::write_os_abi),
      ELFWriter::InstanceMethod("writeString", &ELFWriter::write_string),
      ELFWriter::InstanceMethod("writeSectionBytes", &ELFWriter::write_section_bytes),
      ELFWriter::InstanceMethod("writeSymbols", &ELFWriter::write_symbols),
      ELFWriter::InstanceMethod("writeRelocations", &ELFWriter::write_relocations),
      ELFWriter::InstanceMethod("dumpToFile", &ELFWriter::dump_to_file),
      ELFWriter::InstanceMethod("getSection", &ELFWriter::get_section),
  });
}

std::tuple<ELFIO::section *, bool> bind::ELFWriter::get_or_create_section(std::string name, bool allow_create) {
  // See if the section already exists
  auto section_it = std::find_if(elf->sections.begin(), elf->sections.end(),
                                 [&name](const auto &item) { return item->get_name() == name; });
  // Either create or reference existing section.
  if (section_it == elf->sections.end())
    if (allow_create)
      return std::make_tuple(elf->sections.add(name), true);
    else
      return std::make_tuple(nullptr, false);
  else
    return std::make_tuple((*section_it).get(), false);
}
ELFIO::Elf_Word bind::ELFWriter::write_string_impl(std::string section_name, std::string value) {

  // Create the string table cache for the named section if it doesn't exist.
  if (const auto section_cache_it = this->string_cache.find(section_name); section_cache_it == string_cache.end()) {
    string_cache[section_name] = {};
  }

  // Above guard guarantees that cache will exist.
  auto &active_cache = this->string_cache[section_name];
  // Check if the string value is in our selected cache
  if (const auto string_cache_it = active_cache.find(value); string_cache_it == active_cache.end()) {

    // Either create or reference existing section.
    auto [section, is_new] = this->get_or_create_section(section_name);
    if (is_new)
      section->set_type(ELFIO::SHT_STRTAB);
    // Add the string value to the section
    auto str_accessor = ELFIO::string_section_accessor(section);
    return active_cache[value] = str_accessor.add_string(value);
  } else
    return active_cache[value];
}