#pragma once
#include "core/math/bitmanip/leb128.hpp"
#include "fmt/format.h"
#include "toolchain/link/mmio.hpp"
#include "toolchain2/asmb/pep_common.hpp"

#include <zpp_bits.h>

namespace ELFIO {
class elfio;
}

namespace pepp::tc {

class DiagnosticTable;

struct SectionDescriptor {
  std::string name;
  pepp::tc::ir::attr::SectionFlags flags;
  u16 alignment = 1, org_count = 0;
  std::optional<u16> base_address = std::nullopt;
  u16 low_address = 0, high_address = 0;
  // Number of bytes that will be written out to ELF file.
  // Not high_address-low_address because .ORG can mess with address!
  u16 byte_count = 0;
  // Symbols need to know which section they are going to be relocated against.
  // Rather than wait until the elf file has been generated, we can verify (through source code inspection!) the number
  // that will be assigned to the first non-ELF-plumbing section.
  // Then, splitting to sections can increment this counter AND update the symbol declaration's links.
  static constexpr u16 section_base_index = 3;
  u16 section_index = section_base_index;
};

static const SectionDescriptor default_descriptor = {.name = ".text",
                                                     .flags = ir::attr::SectionFlags(true, true, true, false)};

struct SectionAnalysisResults {
  std::vector<std::pair<SectionDescriptor, PepIRProgram>> grouped_ir;
  std::vector<std::string> system_calls;
  std::vector<obj::IO> mmios;
};

// The returned vector points to the same underlying IR as the (linear) input program.
// This allows addresses to be propogated to input original. which is useful for generating the listing.
//
// Also extracts system-calls and memory-mapped IO declarations since this is the one time we iterate overthe whole IR
// at once.
SectionAnalysisResults split_to_sections(DiagnosticTable &diag, PepIRProgram &prog,
                                         SectionDescriptor initial_section = default_descriptor);

// assign_addresses iterates over sections from prog, grouping non-ORG sections contiguously with the nearest ORG
// section to its left. Sections before the first .ORG are an exception, and are grouped with the nearest .ORG to the
// right. When no .ORG is present, all sections are treated as a single group starting at initial_base_address.
//
// For section after a .ORG, the new <base_address> for each section in a section group is the <group_base_address>
// plus the cumulative size of the processed sections in that group after the .ORG, respecting aligment requirements.
// For sections before a .ORG, the new <base_address> is the .ORG minus the cumulative size, iterating right-to-left
// starting in the .ORG section.
//
// When a .BURN <num> is present, grouping occurs as-if an extra section was append to prog which contains a .ORG <num>.
IRMemoryAddressTable assign_addresses(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog,
                                      u16 initial_base_address = 0);

// Create a lookup data structure that converts IR pointers back to the generated object code.
// Since IR no longer know their own address, we need to cache the object code because it cannot easily be regenerated.
using IR2ObjectPair = std::pair<const ir::LinearIR *, bits::span<u8>>;
struct IR2ObjectComparator {
  bool operator()(const IR2ObjectPair &lhs, const IR2ObjectPair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(ir::LinearIR *const lhs, ir::LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const ir::LinearIR *const lhs, const ir::LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ObjectCodeMap = fc::flat_map<std::vector<IR2ObjectPair>, IR2ObjectComparator>;
struct PepStaticRelocation {
  // Offset into a section's object code (in bytes) which needs relocation.
  // Per ELF spec, needs to be offset for relocatable object files rather than an address to simplify linker.
  u32 section_offset;
  u32 section_idx; // section index in prog, not ELF.
};

struct ProgramObjectCodeResult {
  IR2ObjectCodeMap ir_to_object_code;
  // Group relocations by symbol rather than by section so that we can write the symbol table and relocations
  // simultaneously.
  std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, PepStaticRelocation> relocations;
  // A common arena for all section's object code
  std::vector<u8> object_code;
  struct SectionSpans {
    bits::span<u8> object_code;
  };
  // Use section indicies from original "prog" and provides only the object code for a particular section descriptor.
  std::vector<SectionSpans> section_spans;
};

ProgramObjectCodeResult to_object_code(const IRMemoryAddressTable &,
                                       std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog);

// Create a lookup data structure that converts IR pointers back to their listing line number.
using IR2ListingLinePair = std::pair<const ir::LinearIR *, u16>;
struct IR2ListingLineComparator {
  bool operator()(const IR2ListingLinePair &lhs, const IR2ListingLinePair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(ir::LinearIR *const lhs, ir::LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const ir::LinearIR *const lhs, const ir::LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ListingLineMap = fc::flat_map<std::vector<IR2ListingLinePair>, IR2ListingLineComparator>;

struct ElfResult {
  std::shared_ptr<ELFIO::elfio> elf;
  // Some sections in the program are not omitted to to ELF file.
  // However, we pre-computed section indices in the ELF file in split_to_sections and used these in the symbol table.
  // [originally computed section index] is the number you need to subtract to get actual index in the output file.
  std::vector<u16> section_offsets;
  IR2ListingLineMap ir_to_listing;
};

ElfResult to_elf(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog, const IRMemoryAddressTable &addrs,
                 const ProgramObjectCodeResult &object_code, const std::vector<obj::IO> &mmios);

// Write out the symbol table and relocations at the same time.
// Otherwise, we would need to convert all of the symbol::Entry* pointers a second time.
void write_symbol_table(ElfResult &elf, pepp::core::symbol::LeafTable &symbol_table, const ProgramObjectCodeResult &oc,
                        const std::string name = ".symtab");

struct BinaryLineMapping {
  uint32_t address = 0;
  // 0 indicates unset; so we start counting from 1
  uint16_t srcLine = 0, listLine = 0;
  std::strong_ordering operator<=>(const BinaryLineMapping &other) const { return address <=> other.address; }
  operator std::string() const {
    static constexpr auto opt = [](uint16_t l) -> std::string {
      if (l == 0) return "    ";
      else return fmt::format("{:4}", l);
    };
    return fmt::format("{:4X}: ({},{})", address, opt(srcLine), opt(listLine));
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

    using zpp::bits::bytes;
    using Span = bits::span<u8>;
    // Max payload size is 11 bytes (32/7 + 2*(16/7)), padded to 12 because I am paranoid.
    constexpr u8 max_size = 12;
    uint8_t buffer[max_size];

    // Ensure that prev is never null for remainder of function.
    BinaryLineMapping defaultPrev{};
    if (prev == nullptr || noDelta) prev = &defaultPrev;

    if (archive.kind() == zpp::bits::kind::out) {
      // Initialize length to 0, will be patched later after all fields are written.
      u8 total_size = 0;
      auto sizePos = archive.position();
      archive(total_size);

      // Write address
      i32 addressDelta = self.address - prev->address;
      auto size = bits::encodeSLEB128(addressDelta, buffer, 0);
      total_size += size;
      if (auto errc = archive(bytes(Span(buffer, size), size)); errc.code != std::errc()) return errc;

      // Write source line
      i16 srcLineDelta = self.srcLine - prev->srcLine;
      size = bits::encodeSLEB128(srcLineDelta, buffer, 0);
      total_size += size;
      if (auto errc = archive(bytes(Span(buffer, size), size)); errc.code != std::errc()) return errc;

      // Write list line
      i16 listLineDelta = self.listLine - prev->listLine;
      size = bits::encodeSLEB128(listLineDelta, buffer, 0);
      total_size += size;
      if (auto errc = archive(bytes(Span(buffer, size), size)); errc.code != std::errc()) return errc;

      // Update total length field
      auto endPos = archive.position();
      archive.reset(sizePos);
      // Must cast or expression will become i32 and clobber address.
      auto ret = archive(static_cast<u8>(total_size | (noDelta ? 0x80 : 0x00)));
      archive.reset(endPos);
      return ret;
    }
    // Only allow reading into nonconst objects
    else if (archive.kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      // Determine total number of bytes for this entry.
      i8 total_size = 0;
      if (zpp::bits::errc errc = archive(total_size); errc.code != std::errc()) return errc;
      else if ((total_size & 0x7F) > max_size) return zpp::bits::errc(std::errc::value_too_large);
      else if ((total_size & 0x7F) == 0) return zpp::bits::errc(std::errc::illegal_byte_sequence);
      // If sign bit is -1, use default-constructed previous instead of passed prev.
      if (total_size & 0x80) total_size &= 0x7F, prev = &defaultPrev;

      // Load all ULEB128 bytes into a buffer, since they are self-delimiting in the LEB parser.
      auto array_view = bits::span<u8>(buffer, total_size);
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
      throw std::logic_error(e);
    }
    const char *const e = "Unreachable";
    throw std::logic_error(e);
  }
};

static inline const auto lineMapStr = ".debug_line";
// Get line mapping section or return nullptr;
ELFIO::section *getLineMappingSection(ELFIO::elfio &elf);
}
