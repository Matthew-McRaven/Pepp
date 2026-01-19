#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>
#include <spdlog/spdlog.h>
#include "core/bitmanip/leb128.hpp"
#include "toolchain/pas/ast/generic/attr_sec.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/operations/generic/combine.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"
#include "toolchain/symbol/entry.hpp"
#include "zpp_bits.h"

namespace pas::obj::common {
ELFIO::section *addStrTab(ELFIO::elfio &elf);
struct BinaryLineMapping {
  uint32_t address = 0;
  // 0 indicates unset; so we start counting from 1
  uint16_t srcLine = 0, listLine = 0;
  std::strong_ordering operator<=>(const BinaryLineMapping &other) const { return address <=> other.address; }
  operator QString() const {
    using namespace Qt::StringLiterals;
    static const auto format = u"%1: (%2,%3)"_s;
    static const auto opt = [](uint16_t l) {
      if (l == 0) return u"    "_s;
      else return u"%1"_s.arg((int)l, 4);
    };
    return format.arg(address, 4, 16).arg(opt(srcLine)).arg(opt(listLine));
  }
  // Use a length-prepended delta encoding to store the difference between the the previous line mapping and the
  // current on a field-by-field basis. In the average case, this reduces the serialized size to 4 bytes (1B len + 3B
  // for 3 fields stored as signed LEB128) from a constant 8B. This space savings is significant because the line
  // mapping is 30% of the overall ELF file size.
  // If prev is nullptr, it will use a default-initialized BinaryLineMapping, which is usually all 0s.
  // noDelta is only used in the encoder. If true, it will set the sign bit
  // of length to 1. If the decoder encounters a negative length, it will use a default-constructed BinaryLineMapping as
  // prev instead of the passed param. This feature is meant to allow naive concatentation of mappings bytes within on
  // ELF section.
  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, BinaryLineMapping *prev = nullptr,
                                             bool noDelta = false) {
    // Wire format is: 1B length, SLEB128 address delta, SLEB128 source line delta, SLEB128 list line delta.
    // Use signed LEB128 so that we do not impose any ordering requirements on LineMappings.
    // E.g., an .ORG may place higher line numbers at a lower address.

    using namespace zpp::bits;
    using Span = bits::span<quint8>;
    // Max payload size is 11 bytes (32/7 + 2*(16/7)), padded to 12 because I am paranoid.
    constexpr quint8 max_size = 12;
    uint8_t buffer[max_size];

    // Ensure that prev is never null for remainder of function.
    BinaryLineMapping defaultPrev{};
    if (prev == nullptr || noDelta) prev = &defaultPrev;

    if (archive.kind() == zpp::bits::kind::out) {
      // Initialize length to 0, will be patched later after all fields are written.
      quint8 total_size = 0;
      auto sizePos = archive.position();
      archive(total_size);

      // Write address
      qint32 addressDelta = self.address - prev->address;
      auto size = bits::encodeSLEB128(addressDelta, buffer, 0);
      total_size += size;
      if (auto errc = archive(bytes(Span(buffer, size), size)); errc.code != std::errc()) return errc;

      // Write source line
      qint16 srcLineDelta = self.srcLine - prev->srcLine;
      size = bits::encodeSLEB128(srcLineDelta, buffer, 0);
      total_size += size;
      if (auto errc = archive(bytes(Span(buffer, size), size)); errc.code != std::errc()) return errc;

      // Write list line
      qint16 listLineDelta = self.listLine - prev->listLine;
      size = bits::encodeSLEB128(listLineDelta, buffer, 0);
      total_size += size;
      if (auto errc = archive(bytes(Span(buffer, size), size)); errc.code != std::errc()) return errc;

      // Update total length field
      auto endPos = archive.position();
      archive.reset(sizePos);
      // Must cast or expression will become i32 and clobber address.
      auto ret = archive(static_cast<quint8>(total_size | (noDelta ? 0x80 : 0x00)));
      archive.reset(endPos);
      return ret;
    }
    // Only allow reading into nonconst objects
    else if (archive.kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      // Determine total number of bytes for this entry.
      quint8 total_size = 0;
      if (zpp::bits::errc errc = archive(total_size); errc.code != std::errc()) return errc;
      else if ((total_size & 0x7F) > max_size) return zpp::bits::errc(std::errc::value_too_large);
      else if ((total_size & 0x7F) == 0) return zpp::bits::errc(std::errc::illegal_byte_sequence);
      // If sign bit is -1, use default-constructed previous instead of passed prev.
      if (total_size & 0x80) total_size &= 0x7F, prev = &defaultPrev;

      // Load all ULEB128 bytes into a buffer, since they are self-delimiting in the LEB parser.
      auto array_view = bits::span<quint8>(buffer, total_size);
      if (auto errc = archive(zpp::bits::bytes(array_view, array_view.size_bytes())); errc.code != std::errc())
        return errc;

      // As we consume LEB bytes, update offset to point to the "end" of the consumed bytes.
      unsigned entrySize = 0, offset = 0;
      // Decode address
      uint64_t entryValue = bits::decodeSLEB128(reinterpret_cast<const uint8_t *>(buffer + offset), &entrySize);
      offset += entrySize;
      self.address = entryValue + prev->address;

      // Decode source line
      entryValue = bits::decodeSLEB128(reinterpret_cast<const uint8_t *>(buffer + offset), &entrySize);
      offset += entrySize;
      self.srcLine = entryValue + prev->srcLine;

      // Decode listing line
      entryValue = bits::decodeSLEB128(reinterpret_cast<const uint8_t *>(buffer + offset), &entrySize);
      self.listLine = entryValue + prev->listLine;
      return std::errc();
    } else if (archive.kind() == zpp::bits::kind::in) {
      const char *const e = "Can't read into const";
      qCritical(e);
      throw std::logic_error(e);
    }
    const char *const e = "Unreachable";
    qCritical(e);
    throw std::logic_error(e);
  }
};

void writeLineMapping(ELFIO::elfio &elf, pas::ast::Node &root);
std::vector<BinaryLineMapping> getLineMappings(ELFIO::elfio &elf);
void writeSymtab(ELFIO::elfio &elf, symbol::Table &table, QString prefix);

template <typename ISA> void writeTree(ELFIO::elfio &elf, pas::ast::Node &node, QString prefix, bool isOS) {
  using namespace pas;
  using namespace Qt::StringLiterals;
  ELFIO::segment *activeSeg = nullptr;

  auto getOrCreateBSS = [&](ast::generic::SectionFlags::Flags &flags) {
    if (activeSeg == nullptr || activeSeg->get_file_size() != 0) {
      activeSeg = elf.segments.add();
      activeSeg->set_type(ELFIO::PT_LOAD);
      ELFIO::Elf_Word elfFlags = 0;
      elfFlags |= flags.R ? ELFIO::PF_R : 0;
      elfFlags |= flags.W ? ELFIO::PF_W : 0;
      elfFlags |= flags.X ? ELFIO::PF_X : 0;
      activeSeg->set_flags(elfFlags);
      activeSeg->set_physical_address(-1);
      activeSeg->set_virtual_address(-1);
    }
    return activeSeg;
  };

  auto getOrCreateBits = [&](ast::generic::SectionFlags::Flags &flags) {
    if (activeSeg == nullptr || activeSeg->get_file_size() == 0 ||
        !(((activeSeg->get_flags() & ELFIO::PF_R) > 0 == flags.R) &&
          ((activeSeg->get_flags() & ELFIO::PF_W) > 0 == flags.W) &&
          ((activeSeg->get_flags() & ELFIO::PF_X) > 0 == flags.X))) {
      activeSeg = elf.segments.add();
      activeSeg->set_type(ELFIO::PT_LOAD);
      ELFIO::Elf_Word elfFlags = 0;
      elfFlags |= flags.R ? ELFIO::PF_R : 0;
      elfFlags |= flags.W ? ELFIO::PF_W : 0;
      elfFlags |= flags.X ? ELFIO::PF_X : 0;
      activeSeg->set_flags(elfFlags);
      activeSeg->set_physical_address(-1);
      activeSeg->set_virtual_address(-1);
    }
    return activeSeg;
  };

  for (auto &astSec : ast::children(node)) {
    // Strip leading . from sections, since we will be adding prefix+".".
    auto secName = astSec->get<ast::generic::SectionName>().value;
    if (secName.startsWith(".")) secName = secName.mid(1);
    auto secFlags = astSec->get<ast::generic::SectionFlags>().value;
    auto fullName = u"%1.%2"_s.arg(prefix, secName).toStdString();
    SPDLOG_INFO("{} creating", fullName);
    auto traits = pas::ops::generic::detail::getTraits(*astSec);
    auto align = traits.alignment;
    ELFIO::Elf64_Addr baseAddr = traits.base;
    auto size = traits.size;
    auto bytes = pas::ops::pepp::toBytes<ISA>(*astSec);
    if (size == 0) continue; // 0-sized sections are meaningless, do not emit.

    auto sec = elf.sections.add(fullName);
    // All sections from AST correspond to bits in Pep/10 memory, so alloc
    auto shFlags = ELFIO::SHF_ALLOC;
    shFlags |= secFlags.X ? ELFIO::SHF_EXECINSTR : 0;
    shFlags |= secFlags.W ? ELFIO::SHF_WRITE : 0;
    sec->set_flags(shFlags);
    sec->set_addr_align(align);
    SPDLOG_TRACE("{} sized at {:x}", fullName, size);

    if (secFlags.Z) {
      SPDLOG_TRACE("{} zeroed", fullName);
      sec->set_type(ELFIO::SHT_NOBITS);
      sec->set_size(size);
    } else {
      SPDLOG_TRACE("{} assigned {:x} bytes", fullName, bytes.size());
      Q_ASSERT(bytes.size() == size);
      sec->set_type(ELFIO::SHT_PROGBITS);
      sec->set_data((const char *)bytes.constData(), size);
    }

    // If we are before the first segment of the OS, add the user address space.
    if (isOS && activeSeg == nullptr) {
      SPDLOG_TRACE("{} created user segment with defaults", fullName);
      auto userSeg = elf.segments.add();
      userSeg->set_align(1);
      userSeg->set_file_size(0);
      userSeg->set_virtual_address(0x0);
      userSeg->set_physical_address(0x0);
      userSeg->set_memory_size(baseAddr);
      userSeg->set_type(ELFIO::PT_LOAD);
      userSeg->set_flags(ELFIO::PF_R | ELFIO::PF_W | ELFIO::PF_X);
    }

    // Both implicitly capture isOS.
    if (!isOS) activeSeg = elf.segments[0];
    else if (secFlags.Z) getOrCreateBSS(secFlags);
    else getOrCreateBits(secFlags);

    activeSeg->add_section(sec, align);
    activeSeg->set_physical_address(std::min(activeSeg->get_physical_address(), baseAddr));
    activeSeg->set_virtual_address(std::min(activeSeg->get_virtual_address(), baseAddr));
    SPDLOG_TRACE("{} base address set to {:x}", fullName, baseAddr);

    // Field not re-computed on its own. Failure to compute will cause readelf to crash.
    // TODO: in the future, handle alignment correctly?
    if (isOS) activeSeg->set_memory_size(activeSeg->get_memory_size() + size);

    // Update the section index of all symbols in this section, otherwise symtab
    // will link against SHN_UNDEF.
    for (auto &line : ast::children(*astSec))
      if (line->has<ast::generic::SymbolDeclaration>())
        line->get<ast::generic::SymbolDeclaration>().value->section_index = sec->get_index();
  }

  if (node.has<ast::generic::SymbolTable>()) writeSymtab(elf, *node.get<ast::generic::SymbolTable>().value, prefix);
}

static inline const auto lineMapStr = ".debug_line";
namespace detail {
// Get line mapping section or return nullptr;
ELFIO::section *getLineMappingSection(ELFIO::elfio &elf);
} // namespace detail
} // namespace pas::obj::common
