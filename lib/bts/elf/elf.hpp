#pragma once

#include <zpp_bits.h>
#include "bts/elf/header.hpp"
#include "bts/elf/section.hpp"
namespace pepp::bts {

template <typename E> class Elf {
public:
  // Create an empty ELF file with the given file type and ABI
  Elf(FileType, ElfABI);
  ElfEhdr<E> header;

  void add_section_header_table() {
    if (!section_headers.empty()) return;
    add_section(create_null_header<E>());
    add_section(create_shstrtab_header<E>(1));

    header.e_shstrndx = 1;
    header.e_shentsize = sizeof(ElfShdr<E>);
    auto &shstrab = section_data.back();
    shstrab.push_back(0);
    shstrab.insert(shstrab.end(), {'.', 's', 'h', 's', 't', 'r', 't', 'a', 'b', 0});
  }

  // TODO: someday, what we actually want to do is mmap the file. The archive will wrap the mmap'ed output, and we will
  // jump around. Purposefully split output from input to avoid some archive trickery.
  template <typename T> constexpr static zpp::bits::errc serialize(zpp::bits::basic_out<T> &archive, auto &self) {
    // Write section header immediately after header
    if (zpp::bits::errc errc = archive(self.header); errc.code != std::errc()) return errc;

    // Skip first section (null), because it has no data.
    for (size_t i = 1; i < self.section_data.size(); ++i) {
      const auto &sdat = self.section_data[i];
      const auto span = std::span(sdat.data(), sdat.size());
      if (zpp::bits::errc errc = archive(zpp::bits::bytes(span, span.size())); errc.code != std::errc()) return errc;
    }

    // Write out section headers, then program headers before EoF
    for (const auto &shdr : self.section_headers)
      if (zpp::bits::errc errc = archive(shdr); errc.code != std::errc()) return errc;

    return {};
  }

  u32 add_section(ElfShdr<E> &&shdr) {
    section_headers.emplace_back(shdr);
    section_data.emplace_back();
    u32 ret = static_cast<u32>(section_headers.size() - 1);
    header.e_shnum = section_headers.size();
    return ret;
  };

  // Place the section header followed by the program header table at the given offset
  u64 place_header_tables_at(u64 off) {
    // Then place section header table
    if (!section_headers.empty()) {
      header.e_shoff = off;
      off += header.e_shentsize * header.e_shnum;
    }
    // Followed by program header table
    if (false) {
      header.e_phoff = off;
      off += header.e_phentsize * header.e_phnum;
    }
    return off;
  }
  void calculate_layout() {
    u64 rolling_offset = sizeof(ElfEhdr<E>);
    // Finalize header fields for program header table
    // Skip first section (null), because we want a 0-offset.
    for (size_t i = 1; i < section_headers.size(); ++i) {
      auto &shdr = section_headers[i];
      shdr.sh_offset = rolling_offset;
      auto size = section_data[i].size();
      shdr.sh_size = size, rolling_offset += size;
    }
    rolling_offset = place_header_tables_at(rolling_offset);
  }

private:
  std::vector<ElfShdr<E>> section_headers;
  std::vector<std::vector<u8>> section_data;
};
template <typename E> pepp::bts::Elf<E>::Elf(FileType type, ElfABI abi) : header(type, abi) {}

} // namespace pepp::bts
