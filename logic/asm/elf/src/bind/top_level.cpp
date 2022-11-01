//
// Created by gpu on 10/27/22.
//
#include "./top_level.hpp"
#include "./utils.hpp"
#include "./section.hpp"
#include "./segment.hpp"
#include <sstream>

bind::Elf::Elf(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<Elf>(info) {
  auto env = info.Env();
  bind::detail::count_args(info, 0, 0);
}

Napi::Value bind::Elf::init(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto bitness = bind::detail::parse_arg_number(info, 0, "number");
  if (bitness != 32 && bitness != 64)
    Napi::TypeError::New(env, "Bitness must be 32 or 64").ThrowAsJavaScriptException();
  elf = std::make_shared<ELFIO::elfio>();
  elf->create(bitness == 32 ? ELFIO::ELFCLASS32 : ELFIO::ELFCLASS64, ELFIO::ELFDATA2MSB);
  return env.Null();
}

Napi::Value bind::Elf::validate(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  auto string = elf->validate();
  if (string == "")
    return Napi::Boolean::New(info.Env(), true);
  else
    return Napi::String::New(info.Env(), string);
}

Napi::Value bind::Elf::get_class(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(elf->get_class()));
}

Napi::Value bind::Elf::get_version(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(elf->get_version()));
}

Napi::Value bind::Elf::get_os_abi(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(elf->get_os_abi()));
}
Napi::Value bind::Elf::set_os_abi(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 1, 1);
  elf->set_os_abi(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}
Napi::Value bind::Elf::get_abi_version(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(elf->get_abi_version()));
}
Napi::Value bind::Elf::set_abi_version(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 1, 1);
  elf->set_abi_version(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Elf::get_type(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(elf->get_type()));
}
Napi::Value bind::Elf::set_type(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 1, 1);
  elf->set_type(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Elf::get_machine(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(elf->get_machine()));
}
Napi::Value bind::Elf::set_machine(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 1, 1);
  elf->set_machine(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Elf::get_flags(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(elf->get_flags()));
}
Napi::Value bind::Elf::set_flags(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 1, 1);
  elf->set_flags(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Elf::get_entry(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), elf->get_entry());
}
Napi::Value bind::Elf::set_entry(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 1, 1);
  elf->set_entry(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Elf::get_default_entry_size(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  bind::detail::count_args(info, 1, 1);
  auto sec = bind::detail::parse_arg_bigint(info, 0, "bigint");
  return Napi::BigInt::New(info.Env(), elf->get_default_entry_size(sec));
}

Napi::Value bind::Elf::add_section(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto ptr = elf->sections.add(bind::detail::parse_arg_string(info, 0, "string"));
  Napi::EscapableHandleScope scope(info.Env());
  auto sec = Section::GetClass(env).New({Napi::External<std::shared_ptr<ELFIO::elfio >>::New(env, &elf),
                                         Napi::External<ELFIO::section>::New(env, ptr)});
  scope.Escape(sec);
  sec_map[ptr->get_index()] = ptr;
  return sec;
}
Napi::Value bind::Elf::get_section(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  if (info[0].IsBigInt()) {
    uint64_t index = bind::detail::parse_arg_bigint(info, 0, "bigint");
    if (auto sec = sec_map.find(index); sec == sec_map.end())
      return env.Undefined();
    else
      return Section::GetClass(env).New({Napi::External<std::shared_ptr<ELFIO::elfio >>::New(env, &elf),
                                         Napi::External<ELFIO::section>::New(env, sec->second)});
  } else if (info[0].IsString()) {
    auto name = bind::detail::parse_arg_string(info, 0, "string");
    for (const auto &it : elf->sections) {
      if (it->get_name() == name) {
        return Section::GetClass(env).New({Napi::External<std::shared_ptr<ELFIO::elfio >>::New(env, &elf),
                                           Napi::External<ELFIO::section>::New(env, &*it)});
      }
    }
    return env.Undefined();
  } else
    return env.Undefined();
}

Napi::Value bind::Elf::add_segment(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  auto env = info.Env();
  bind::detail::count_args(info, 0, 0);
  auto ptr = elf->segments.add();
  Napi::EscapableHandleScope scope(info.Env());
  auto seg = Segment::GetClass(env).New({Napi::External<std::shared_ptr<ELFIO::elfio >>::New(env, &elf),
                                         Napi::External<ELFIO::segment>::New(env, ptr)});

  scope.Escape(seg);
  seg_map[ptr->get_index()] = ptr;
  return seg;
}
Napi::Value bind::Elf::get_segment(const Napi::CallbackInfo &info) {
  validate_elf_ptr(info);
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  uint64_t index = bind::detail::parse_arg_bigint(info, 0, "bigint");
  if (auto seg = seg_map.find(index); seg == seg_map.end())
    return env.Undefined();
  else
    return Segment::GetClass(env).New({Napi::External<std::shared_ptr<ELFIO::elfio >>::New(env, &elf),
                                       Napi::External<ELFIO::segment>::New(env, seg->second)});
}

Napi::Function bind::Elf::GetClass(Napi::Env env) {
  return bind::Elf::DefineClass(env, "Elf", {
      Elf::InstanceMethod("init", &Elf::init),
      Elf::InstanceMethod("validate", &Elf::validate),
      Elf::InstanceMethod("getClass", &Elf::get_class),
      Elf::InstanceMethod("getVersion", &Elf::get_version),
      Elf::InstanceMethod("getOSABI", &Elf::get_os_abi),
      Elf::InstanceMethod("setOSABI", &Elf::set_os_abi),
      Elf::InstanceMethod("getABIVersion", &Elf::get_abi_version),
      Elf::InstanceMethod("setABIVersion", &Elf::set_abi_version),
      Elf::InstanceMethod("getType", &Elf::get_type),
      Elf::InstanceMethod("setType", &Elf::set_type),
      Elf::InstanceMethod("getMachine", &Elf::get_machine),
      Elf::InstanceMethod("setMachine", &Elf::set_machine),
      Elf::InstanceMethod("getFlags", &Elf::get_flags),
      Elf::InstanceMethod("setFlags", &Elf::set_flags),
      Elf::InstanceMethod("getEntry", &Elf::get_entry),
      Elf::InstanceMethod("setEntry", &Elf::set_entry),
      Elf::InstanceMethod("getDefaultEntrySize", &Elf::get_default_entry_size),
      Elf::InstanceMethod("addSection", &Elf::add_section),
      Elf::InstanceMethod("getSection", &Elf::get_section),
      Elf::InstanceMethod("addSegment", &Elf::add_segment),
      Elf::InstanceMethod("getSegment", &Elf::get_segment),
  });
}

void bind::Elf::validate_elf_ptr(const Napi::CallbackInfo &info) {
  if (elf == nullptr)
    Napi::TypeError::New(info.Env(), "Call init() before using class").ThrowAsJavaScriptException();
}

Napi::Value bind::save_file(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 2, 2);
  auto *elf = bind::detail::parse_arg_wrapped<bind::Elf>(info, 0, "Elf");
  auto path = bind::detail::parse_arg_string(info, 1, "string");
  ELFIO::elfio *elfio = elf->get_elfio();
  auto result = elfio->save(path);
  return Napi::Boolean::New(env, result);
}

Napi::Value bind::load_file(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto path = bind::detail::parse_arg_string(info, 0, "string");
  auto obj = bind::Elf::GetClass(env).New({Napi::Number::New(env, 64)});
  auto elf = Napi::ObjectWrap<bind::Elf>::Unwrap(obj);
  if (!elf->get_elfio()->load(path))
    Napi::TypeError::New(info.Env(), "Failed to load ELF file").ThrowAsJavaScriptException();
  return obj;
}

Napi::Value bind::save_buffer(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  bind::detail::count_args(info, 1, 1);
  auto elf_js = Elf::GetClass(env).New({info[0]});
  auto elf_native = Napi::ObjectWrap<bind::Elf>::Unwrap(elf_js);
  ELFIO::elfio *elfio = elf_native->get_elfio();
  std::stringstream os;
  if (!elfio->save(os))
    Napi::TypeError::New(info.Env(), "Failed to save to buffer").ThrowAsJavaScriptException();
  const auto &bytes = os.str();
  // TODO: Implement when I have data access.
  // bytes.data();
  // const buffer = Napi::ArrayBuffer()
  return Napi::Boolean::New(env, true);
}

/*Napi::Value bind::load_buffer(const Napi::CallbackInfo &info) {

}*/
