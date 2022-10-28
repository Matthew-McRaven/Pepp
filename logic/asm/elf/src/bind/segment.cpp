#include "segment.hpp"
#include "section.hpp"
#include "utils.hpp"

bind::Segment::Segment(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<Segment>(info) {
  bind::detail::count_args(info, 2, 2);
  this->elf =
      *bind::detail::parse_arg_external<std::shared_ptr<ELFIO::elfio>>(info, 0, "shared pointer to an elf file");
  this->segment = bind::detail::parse_arg_external<ELFIO::segment>(info, 1, "raw pointer to an elf segment");
}

Napi::Value bind::Segment::get_index(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_index()));
}

Napi::Value bind::Segment::get_type(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_type()));
}

Napi::Value bind::Segment::set_type(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  segment->set_type(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Segment::get_align(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_align()));
}

Napi::Value bind::Segment::set_align(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  segment->set_align(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Segment::get_vaddress(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_virtual_address()));
}

Napi::Value bind::Segment::set_vaddress(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  segment->set_virtual_address(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Segment::get_paddress(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_physical_address()));
}

Napi::Value bind::Segment::set_paddress(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  segment->set_physical_address(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Segment::get_memory_size(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_memory_size()));
}

Napi::Value bind::Segment::set_memory_size(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  segment->set_memory_size(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Segment::get_file_size(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_file_size()));
}

Napi::Value bind::Segment::get_flags(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_flags()));
}

Napi::Value bind::Segment::set_flags(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  segment->set_flags(bind::detail::parse_arg_bigint(info, 0, "bigint"));
  return info.Env().Null();
}

Napi::Value bind::Segment::get_section_count(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_sections_num()));
}

Napi::Value bind::Segment::get_section_index_at(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  auto arrNdx = bind::detail::parse_arg_bigint(info, 0, "bigint");
  return Napi::BigInt::New(info.Env(), static_cast<uint64_t>(segment->get_section_index_at(arrNdx)));
}

Napi::Value bind::Segment::add_section(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  auto external_section = bind::detail::parse_arg_wrapped<bind::Section>(info, 0, "ISection");
  auto as_section_ptr = external_section->get_raw_Section();
  segment->add_section(as_section_ptr, as_section_ptr->get_addr_align());
  recompute_file_size_impl();
  return info.Env().Null();
}

Napi::Value bind::Segment::add_section_index(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 1, 1);
  auto shndx = bind::detail::parse_arg_bigint(info, 0, "bigint");
  segment->add_section_index(shndx, elf->sections[shndx]->get_addr_align());
  return info.Env().Null();
}

Napi::Value bind::Segment::recompute_file_size(const Napi::CallbackInfo &info) {
  bind::detail::count_args(info, 0, 0);
  recompute_file_size_impl();
  return info.Env().Null();
}

Napi::Function bind::Segment::GetClass(Napi::Env env) {
  return bind::Segment::DefineClass(env, "Segment", {
      Segment::InstanceMethod("getIndex", &Segment::get_index),
      Segment::InstanceMethod("getType", &Segment::get_type),
      Segment::InstanceMethod("setType", &Segment::set_type),
      Segment::InstanceMethod("getAlign", &Segment::get_align),
      Segment::InstanceMethod("setAlign", &Segment::set_align),
      Segment::InstanceMethod("getVAddress", &Segment::get_paddress),
      Segment::InstanceMethod("setVAddress", &Segment::set_vaddress),
      Segment::InstanceMethod("getPAddress", &Segment::get_paddress),
      Segment::InstanceMethod("setPAddress", &Segment::set_paddress),
      Segment::InstanceMethod("getMemorySize", &Segment::get_memory_size),
      Segment::InstanceMethod("setMemorySize", &Segment::set_memory_size),
      Segment::InstanceMethod("getFileSize", &Segment::get_file_size),
      Segment::InstanceMethod("getFlags", &Segment::get_flags),
      Segment::InstanceMethod("setFlags", &Segment::set_flags),
      Segment::InstanceMethod("getSectionCount", &Segment::get_section_count),
      Segment::InstanceMethod("getSectionIndexAt", &Segment::get_section_index_at),
      Segment::InstanceMethod("addSection", &Segment::add_section),
      Segment::InstanceMethod("addSectionIndex", &Segment::add_section_index),
      Segment::InstanceMethod("recomputeFileSize", &Segment::recompute_file_size),
  });
}

void bind::Segment::recompute_file_size_impl() {
  ELFIO::Elf_Xword accumulate = 0;
  for (ELFIO::Elf_Half secNum = 0; secNum < segment->get_sections_num(); secNum++) {
    auto shndx = segment->get_section_index_at(secNum);
    accumulate += elf->sections[shndx]->get_size();
  }
  segment->set_file_size(accumulate);
}