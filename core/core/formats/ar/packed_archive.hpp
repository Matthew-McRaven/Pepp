/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <cstddef>
#include <expected>
#include <filesystem>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "core/formats/byte_cursor.hpp"
#include "core/formats/elf/packed_access_headers.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/layout_item.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/log2.hpp"
#include "core/math/bitmanip/span.hpp"

namespace pepp::bts {
struct AElfSource;
struct AStorage;

#pragma pack(push, 1)
// One member header matching the on-disk layout.
// There's no official specification. I've opted to be compatible with llvm-ar and mold, which means the header format
// is as follows. All fields technically contain ASCII text, so member methods exist to convert from space-padded text
// to the real data type. See llvm's definition at:
// https://github.com/llvm/llvm-project/blob/llvmorg-23.1.4/llvm/include/llvm/Object/Archive.h#L96-L104
struct PackedArHeader {
  // Required value of the last two bytes of each header.
  inline static constexpr std::string_view FMAG = "`\n";
  // If the real value of ar_name is longer than 15 bytes or contains a / it is actually stored in the // table.
  // In that case, name contains /<decimal>. Once parsed, that decimal is a byte offset into the name table.
  char ar_name[16];
  char ar_date[12];
  char ar_uid[6];
  char ar_gid[6];
  char ar_mode[8];
  char ar_size[10]; // Data size excluding header. For non-special members, also excludes padding.
  char ar_fmag[2];

  PackedArHeader() = default;
  // Caller is responsible for ensuring name and size fit in 16 and 10 byte fields, respectively.
  // If mode is non-nullopt, uid/gid/date are set to 0, like the deterministic mode of `ar D`.
  // If mode is set to nullopt, then date...mode will be only spaces.
  PackedArHeader(std::string_view raw_name, u64 size, std::optional<u32> mode = 0644) noexcept;

  bool has_valid_fmag() const noexcept;
  // ar_name without its space padding, such as "a.o/", "/0", or "//".
  std::string_view raw_name() const noexcept;
  // The two special members, which hold the symbol index and the long-name table.
  bool is_symbol_index() const noexcept { return raw_name() == "/"; }
  bool is_long_names() const noexcept { return raw_name() == "//"; }
  std::optional<u64> date() const noexcept;
  // As in LLVM, a blank uid or gid reads as 0.
  std::optional<u32> uid() const noexcept;
  std::optional<u32> gid() const noexcept;
  // Octal.
  std::optional<u32> mode() const noexcept;
  std::optional<u64> size() const noexcept;
};
#pragma pack(pop)

inline constexpr u64 ar_header_size = sizeof(PackedArHeader);
static_assert(ar_header_size == 60);

struct ArchiveError {
  enum class Code : u8 {
    CannotOpen,
    BadMagic,
    TruncatedHeader,
    BadFmag,
    BadSize,
    BadDate,
    BadUid,
    BadGid,
    BadMode,
    MemberPastEnd,
    EmptyName,
    BadLongNameOffset,
    MissingLongNames,
    DuplicateLongNames,
    MisplacedLongNames,
    LongNameOutOfRange,
    UnterminatedLongName,
    MisplacedSymbolIndex,
    UnsupportedBsdName,
    UnsupportedBsdSymbolTable,
    UnsupportedSymbolIndex64,
    // Contents of `/`.
    SymbolIndexTruncated,
    SymbolIndexCountTooLarge,
    UnterminatedSymbolName,
    // From open_member_elf.
    ThinMember,
    NotElf,
    // From PackedGrowableArchive.
    NameHasNewline,
    MemberTooLarge,
    NeedsSymbolIndex64,
  };
  // A short, lower-case description, such as "truncated member header".
  static std::string_view to_string(Code code);

  Code code;
  // Offset into the archive where the problem was found.
  u64 offset = 0;
};

// A partially-decoded member header. The name / size have been decoded, but other properties of header are computed as
// needed.
struct PackedArMember {
  PackedArHeader header;
  // Number of bytes in the member's name
  u32 name_length = 0;
  // Offset from start of the archive to the first byte of the member's name.
  u64 name_offset = 0;
  u64 header_offset = 0;
  // Decoded from header. For a thin member, the size of the external file.
  u64 size = 0;
  u64 data_offset() const noexcept { return header_offset + ar_header_size; }
};

// An entry of the / table, which is a global symbol definition.
struct ArSymbol {
  std::string_view name;
  // Absolute file offset to the member header which defines this symbol.
  u64 header_offset = 0;
  // Absolute file offset of this entry in the / table.
  u64 entry_offset = 0;
};

class PackedInputArchive;

// Walks the regular members in file order. An error is yielded once, after which the iterator is at its end.
class ArMemberIterator {
public:
  using value_type = std::expected<PackedArMember, ArchiveError>;
  using difference_type = std::ptrdiff_t;

  ArMemberIterator() = default;
  const value_type &operator*() const noexcept { return _current; }
  const value_type *operator->() const noexcept { return &_current; }
  ArMemberIterator &operator++();
  void operator++(int) { ++*this; }
  bool operator==(std::default_sentinel_t) const noexcept { return _archive == nullptr; }

private:
  friend class PackedInputArchive;
  // The cursor starts at the first regular member's header.
  ArMemberIterator(const PackedInputArchive &archive, ByteCursor cursor);
  const PackedInputArchive *_archive = nullptr;
  ByteCursor _cursor;
  value_type _current;
};

// Walks the symbols of `/`. An error is yielded once, after which the iterator is at its end.
// The layout of / is as follows: a big-endian u32 count, followed by count*be u32 offsets, followed by count
// null-terminated strings. If the final size would be odd, a \n byte is appended.
class ArSymbolIterator {
public:
  using value_type = std::expected<ArSymbol, ArchiveError>;
  using difference_type = std::ptrdiff_t;

  ArSymbolIterator() = default;
  const value_type &operator*() const noexcept { return _current; }
  const value_type *operator->() const noexcept { return &_current; }
  ArSymbolIterator &operator++();
  void operator++(int) { ++*this; }
  bool operator==(std::default_sentinel_t) const noexcept { return _archive == nullptr; }

private:
  friend class PackedInputArchive;
  // Throws if archive does not contain a member named /
  explicit ArSymbolIterator(const PackedInputArchive &archive);
  const PackedInputArchive *_archive = nullptr;
  // One reads the header offsets, the other the names that follow them.
  ByteCursor _offsets, _names;
  u64 _remaining = 0;
  value_type _current;
};

// A read-only archive (of `ar` fame) over a collection of bytes. The first two members are expected to be the special
// sections / and //. From there, it typically contains ELF files as members. Other data may be stored as a member, but
// only ELF files will be registered in the symbol index /. Typically mapped into memory eagerly, pages are loaded in on
// first use. Very little data is cached inside thise cache, iterators always read and parse the archive on each
// iteration. Errors are returned via expected.
class PackedInputArchive {
public:
  inline static constexpr std::string_view REGULAR_MAGIC = "!<arch>\n";
  inline static constexpr std::string_view THIN_MAGIC = "!<thin>\n";

  // Checks the magic and the leading `/` and `//`. Throws is source is nullptr.
  static std::expected<PackedInputArchive, ArchiveError> open(std::shared_ptr<const AElfSource> source);
  static std::expected<PackedInputArchive, ArchiveError> open(const std::string &path);

  bool thin() const noexcept { return _thin; }
  u64 size() const noexcept { return _bytes.size(); }
  // Access a subspan of the archive, or empty if out-of-bounds.
  bits::span<const u8> bytes(u64 offset, u64 length) const noexcept;
  // Contents of `/` and `//`, or nullopt if the archive has none.
  std::optional<LayoutItem> symbol_index() const noexcept { return _symbol_index; }
  std::optional<LayoutItem> long_names() const noexcept { return _long_names; }

  // Iterators borrow this archive, which must outlive them.
  std::ranges::subrange<ArMemberIterator, std::default_sentinel_t> members() const;
  std::ranges::subrange<ArSymbolIterator, std::default_sentinel_t> symbols() const;

  // From the header or the `//` table, without its `/` terminator. A thin member's name is its path.
  std::string_view name(const PackedArMember &member) const;
  // Empty for a thin member.
  bits::span<const u8> data(const PackedArMember &member) const noexcept;
  // Access the underlying storage for a member
  std::shared_ptr<const AStorage> member_storage(const PackedArMember &member) const;

private:
  friend class ArMemberIterator;
  friend std::expected<AnyInputElf, ArchiveError> open_member_elf(const PackedInputArchive &, const PackedArMember &);
  // The member's bytes as a source for open_input_elf. A thin member has none.
  std::expected<std::shared_ptr<const AElfSource>, ArchiveError> member_source(const PackedArMember &member) const;
  PackedInputArchive() = default;
  // The member whose header is at the cursor, leaving the cursor at the next header.
  std::expected<PackedArMember, ArchiveError> read_member(ByteCursor &cursor) const;
  std::shared_ptr<const AElfSource> _source;
  // Keeps the mapping behind _bytes alive.
  std::shared_ptr<const AStorage> _contents;
  bits::span<const u8> _bytes;
  bool _thin = false;
  u64 _first_member = 0;
  std::optional<LayoutItem> _symbol_index, _long_names;
};
static_assert(std::input_iterator<ArMemberIterator>);
static_assert(std::input_iterator<ArSymbolIterator>);

// Coerce a member to an ELF file if the archive is not thin. That ELF file will share the same memory-mapped backing as
// the archive.
std::expected<AnyInputElf, ArchiveError> open_member_elf(const PackedInputArchive &archive,
                                                         const PackedArMember &member);

// An incrementally-constructible archive. The symbol index and name tables are inserted as needed, but will be the
// first members of the serialized result if present. Member's data is shared with the callers.
// data is shared, not copied. Layouts point into this object and its member data, so they are valid until either
// changes.
class PackedGrowableArchive {
public:
  explicit PackedGrowableArchive(bool thin = false) : _thin(thin) {}

  bool thin() const noexcept { return _thin; }
  // For thin archives, names are always sent to //. Otherwise, try to fit the name in the header.
  // Data must be non-null. Use NullStorage if you a zero-length member. symbols are the globals of this new member that
  // should be contributed to the symbol index. `symbols` are the names this member defines, in symbol-table order.
  // Returns the newly-added member's index. The archive is unchanged on failure.
  std::expected<u32, ArchiveError> add_member(std::string_view name, std::shared_ptr<const AStorage> data,
                                              bits::span<const std::string_view> symbols = {});

  u32 member_count() const noexcept { return _members.size(); }

  // Return the the file locations for the file magic value, special headers (if present), and member headers + data.
  std::expected<std::vector<LayoutItem>, ArchiveError> calculate_layout();

private:
  struct Member {
    std::string raw_name; // As written in ar_name, such as "a.o/" or "/0".
    std::shared_ptr<const AStorage> data;
    PackedArHeader header; // Rebuilt by calculate_layout.
  };
  // The / member
  struct SymbolIndex {
    // NUL-terminated strings, inserted in the order of add_member(symbols).
    // TODO: could pool these strings to avoid duplicates.
    BlockStorage names;
    ub32 count;                // Number of entries in index, computed in calculate_layout
    std::vector<u32> members;  // The *index* of the member that defines each name.
    std::vector<ub32> offsets; // Relative offset into names for the [i]th symbol of members.
    PackedArHeader header;     // Rebuilt by calculate_layout
    u64 size() const noexcept; // The table's size, including its pad byte.
  };
  // The // member
  struct LongNames {
    BlockStorage entries;  // "name/\n" for each long name.
    PackedArHeader header; // Rebuilt by calculate_layout.
    // The table's size, including its pad byte, as GNU counts it.
    u64 size() const noexcept { return bits::align_up<u64>(entries.size(), 2); }
  };
  u64 first_regular_member_offset() const noexcept;
  // Header plus stored bytes, padded to an even length.
  u64 member_size(const Member &member) const noexcept;

  bool _thin = false;
  std::vector<Member> _members; // Regular members, in file order.
  SymbolIndex _symbols;
  LongNames _long_names;
};

std::expected<std::vector<u8>, ArchiveError> archive_bytes(PackedGrowableArchive &archive);
std::expected<void, ArchiveError> write_archive(PackedGrowableArchive &archive, std::ostream &out);
// Serializes into memory and reads the result back.
std::expected<PackedInputArchive, ArchiveError> to_input_archive(PackedGrowableArchive &archive);

// Combine the archive's path with the member's name to resolve the actual filename of a thin member.
std::filesystem::path thin_member_path(const std::filesystem::path &archive_path, std::string_view member_name);
} // namespace pepp::bts
