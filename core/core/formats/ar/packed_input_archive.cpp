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

#include "core/formats/ar/packed_archive.hpp"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <stdexcept>
#include <system_error>
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
using namespace pepp::bts;
using enum ArchiveError::Code;

using bits::as_string_view;

std::string_view trim_spaces(std::string_view text) { return bits::rtrimmed_view(text, ' '); }

// Number must be followed by spaces otherwise it's a parse failure
template <std::integral T> std::optional<T> parse_number(std::string_view text, int base) {
  return bits::from_chars_exact<T>(trim_spaces(text), base);
}

std::optional<u32> parse_id(std::string_view text) {
  return trim_spaces(text).empty() ? std::optional<u32>(0) : parse_number<u32>(text, 10);
}

std::unexpected<ArchiveError> fail(ArchiveError::Code code, u64 offset) {
  return std::unexpected(ArchiveError{code, offset});
}

// GNU long names end in "/\n".
std::expected<std::string_view, ArchiveError::Code> long_name(std::string_view table, u64 at) {
  if (at >= table.size()) return std::unexpected(LongNameOutOfRange);
  const auto end = table.find('\n', at);
  if (end == std::string_view::npos || end == at || table[end - 1] != '/') return std::unexpected(UnterminatedLongName);
  else if (end - 1 == at) return std::unexpected(EmptyName);
  return table.substr(at, end - 1 - at);
}

//  Names can either be short and fit in ar_name, or they are long and stored in an auxillary table called //. When
// the full name is stored in //, the header's ar name is "/<decimal>", where <decimal> is the byte offset into the //
// table. Either way, if successful update the name_offset and name_length fields of member, which are used to extract a
// string view of the header's name. Returns an error for other special member names that we do not support.
std::expected<void, ArchiveError::Code> decode_name(PackedArMember &member,
                                                   const std::optional<LayoutItem> &long_names) {
  const auto field = as_string_view(member.header.ar_name);
  if (member.header.is_symbol_index() || member.header.is_long_names()) return {};
  else if (field.starts_with('/')) {
    const auto raw = member.header.raw_name();
    if (raw == "/SYM64/") return std::unexpected(UnsupportedSymbolIndex64);
    const auto offset = parse_number<u64>(raw.substr(1), 10);
    if (!offset) return std::unexpected(BadLongNameOffset);
    // `//` must be written before any member that refers to it.
    else if (!long_names) return std::unexpected(MissingLongNames);
    const auto name = long_name(as_string_view(long_names->data), *offset);
    if (!name) return std::unexpected(name.error());
    member.name_offset = long_names->offset + *offset, member.name_length = name->size();
    return {};
  } else if (field.starts_with("#1/")) return std::unexpected(UnsupportedBsdName);
  // Name's containing / must be emitted to the // table, because / is used as a terminator in this context.
  const auto slash = field.find('/');
  const auto name = slash == std::string_view::npos ? trim_spaces(field) : field.substr(0, slash);
  if (name == "__.SYMDEF" || name == "__.SYMDEF SORTED") return std::unexpected(UnsupportedBsdSymbolTable);
  else if (name.empty()) return std::unexpected(EmptyName);
  member.name_offset = member.header_offset, member.name_length = name.size();
  return {};
}

// Reads the next member header at the cursor, skipping over that member's bytes, leaving the cursor at the
// next member header. Validates the header's size, alignment, fmag, and validates that the member's data is smaller
// than the cursor.
std::expected<PackedArMember, ArchiveError> read_raw(ByteCursor &c, bool thin,
                                                     const std::optional<LayoutItem> &long_names) {
  const u64 at = c.position();
  PackedArMember ret{.header = c.fixed<PackedArHeader>(), .header_offset = at};
  if (!c.ok()) return fail(TruncatedHeader, at);
  else if (!ret.header.has_valid_fmag()) return fail(BadFmag, at);
  const auto size = ret.header.size();
  if (!size) return fail(BadSize, at);
  ret.size = *size;
  if (const auto name = decode_name(ret, long_names); !name) return fail(name.error(), at);

  // A thin archive stores only its `/` and `//` contents.
  const bool special = ret.header.is_symbol_index() || ret.header.is_long_names();
  c.bytes(!thin || special ? *size : 0);
  if (!c.ok()) return fail(MemberPastEnd, at);
  // Headers start on even offsets. Like mold, accept a final member that lacks its pad byte.
  if (c.position() % 2 != 0 && c.remaining() != 0) c.bytes(1);
  return ret;
}
} // namespace

std::string_view pepp::bts::ArchiveError::to_string(Code code) {
  switch (code) {
  case CannotOpen: return "cannot open file";
  case BadMagic: return "not an ar archive";
  case TruncatedHeader: return "truncated member header";
  case BadFmag: return "member header does not end in \"`\\n\"";
  case BadSize: return "member size is not a decimal number";
  case BadDate: return "member date is not a decimal number";
  case BadUid: return "member uid is not a decimal number";
  case BadGid: return "member gid is not a decimal number";
  case BadMode: return "member mode is not an octal number";
  case MemberPastEnd: return "member extends past the end of the archive";
  case EmptyName: return "member has an empty name";
  case BadLongNameOffset: return "long name offset is not a decimal number";
  case MissingLongNames: return "long name without a // table";
  case DuplicateLongNames: return "archive has more than one // table";
  case MisplacedLongNames: return "// table follows a regular member";
  case LongNameOutOfRange: return "long name offset is past the end of the // table";
  case UnterminatedLongName: return "long name is not terminated by \"/\\n\"";
  case MisplacedSymbolIndex: return "symbol index is not the first member";
  case UnsupportedBsdName: return "BSD-style member names are not supported";
  case UnsupportedBsdSymbolTable: return "BSD symbol tables are not supported";
  case UnsupportedSymbolIndex64: return "64-bit symbol index (/SYM64/) is not supported";
  case SymbolIndexTruncated: return "symbol index is too small to hold its count";
  case SymbolIndexCountTooLarge: return "symbol index offsets extend past its end";
  case UnterminatedSymbolName: return "symbol index names are truncated";
  case ThinMember: return "thin archive member without an archive path";
  case NotElf: return "member is not an ELF file";
  case NameHasNewline: return "member name contains a newline";
  case MemberTooLarge: return "member is too large for its 10-digit size field";
  case NeedsSymbolIndex64: return "member offsets need a 64-bit symbol index (/SYM64/), which is not supported";
  }
  return "unknown archive error";
}

namespace {
// Left-aligned and space-padded
template <size_t N> void put_field(char (&field)[N], std::string_view text) noexcept {
  std::ranges::fill(field, ' ');
  std::ranges::copy(text.substr(0, N), field);
}

template <size_t N> void put_number(char (&field)[N], u64 value, int base) noexcept {
  char digits[20];
  const auto end = std::to_chars(std::begin(digits), std::end(digits), value, base).ptr;
  put_field(field, std::string_view(digits, end));
}
} // namespace

pepp::bts::PackedArHeader::PackedArHeader(std::string_view raw_name, u64 size, std::optional<u32> mode) noexcept {
  const std::string_view fill = mode ? "0" : "";
  put_field(ar_name, raw_name);
  put_field(ar_date, fill);
  put_field(ar_uid, fill);
  put_field(ar_gid, fill);
  if (mode) put_number(ar_mode, *mode, 8);
  else put_field(ar_mode, "");
  put_number(ar_size, size, 10);
  put_field(ar_fmag, FMAG);
}

std::string_view pepp::bts::PackedArHeader::raw_name() const noexcept { return trim_spaces(as_string_view(ar_name)); }

bool pepp::bts::PackedArHeader::has_valid_fmag() const noexcept { return as_string_view(ar_fmag) == FMAG; }

std::optional<u64> pepp::bts::PackedArHeader::date() const noexcept {
  return parse_number<u64>(as_string_view(ar_date), 10);
}

std::optional<u32> pepp::bts::PackedArHeader::uid() const noexcept { return parse_id(as_string_view(ar_uid)); }

std::optional<u32> pepp::bts::PackedArHeader::gid() const noexcept { return parse_id(as_string_view(ar_gid)); }

std::optional<u32> pepp::bts::PackedArHeader::mode() const noexcept { return parse_number<u32>(as_string_view(ar_mode), 8); }

std::optional<u64> pepp::bts::PackedArHeader::size() const noexcept {
  return parse_number<u64>(as_string_view(ar_size), 10);
}

pepp::bts::ArMemberIterator::ArMemberIterator(const PackedInputArchive &archive, ByteCursor cursor)
    : _archive(&archive), _cursor(cursor) {
  // Does not initially point to a member, so we must advance so that begin() can point to the first element.
  ++*this;
}

pepp::bts::ArMemberIterator &pepp::bts::ArMemberIterator::operator++() {
  if (_archive == nullptr) return *this;
  else if (!_current || _cursor.remaining() == 0) {
    _archive = nullptr;
    return *this;
  } else {
    _current = _archive->read_member(_cursor);
    return *this;
  }
}

pepp::bts::ArSymbolIterator::ArSymbolIterator(const PackedInputArchive &archive) : _archive(&archive) {
  // Big-endian count, followed by "count" big-endian header offsets, then "count" NUL-terminated names.
  const auto index = *archive.symbol_index();
  ByteCursor c(index);
  const u32 count = c.fixed<ub32>();
  if (!c.ok()) {
    _current = fail(SymbolIndexTruncated, index.offset);
    return;
  } else if (count > c.remaining() / sizeof(ub32)) {
    _current = fail(SymbolIndexCountTooLarge, index.offset);
    return;
  }
  _offsets = _names = c;
  _names.offset += sizeof(ub32) * count;
  _remaining = count;
  // Does not initially point to a member, so we must advance so that begin() can point to the first element.
  ++*this;
}

pepp::bts::ArSymbolIterator &pepp::bts::ArSymbolIterator::operator++() {
  if (_archive == nullptr) return *this;
  else if (!_current || _remaining == 0) {
    _archive = nullptr;
    return *this;
  } else {
    // The count was checked against the table during construction, so only the names can run out.
    const u64 entry = _offsets.position();
    const u32 header_offset = _offsets.fixed<ub32>();
    const u64 name_at = _names.position();
    const auto name = _names.cstr();
    if (!_names.ok()) _current = fail(UnterminatedSymbolName, name_at);
    else _current = ArSymbol{name, header_offset, entry}, --_remaining;
    return *this;
  }
}

std::expected<pepp::bts::PackedInputArchive, pepp::bts::ArchiveError>
pepp::bts::PackedInputArchive::open(std::shared_ptr<const AElfSource> source,
                                    std::optional<std::filesystem::path> path) {
  if (!source) throw std::invalid_argument("PackedInputArchive: source must be non-null");
  PackedInputArchive ret;
  ret._source = std::move(source);
  ret._path = std::move(path);
  ret._contents = ret._source->slice(0, ret._source->size());
  // Archive's bytes are lazily paged in on first access for MemoryMapped source.
  ret._bytes = ret._contents->get(0, ret._contents->size());
  ByteCursor c(LayoutItem{0, ret._bytes});

  // Validate that the archive stats with a valid magic number
  const auto magic = as_string_view(c.bytes(REGULAR_MAGIC.size()));
  if (magic == THIN_MAGIC) ret._thin = true;
  else if (magic != REGULAR_MAGIC) return fail(BadMagic, 0);

  // By convention, the first two members of an archive are the symbol index / and the name table //.
  // Read the initial sections until a regular member is found. That becomes the value of _first_member.
  while (c.remaining() != 0) {
    const u64 at = c.position();
    auto next = c;
    const auto member = read_raw(next, ret._thin, ret._long_names);
    if (!member) return std::unexpected(member.error());
    const LayoutItem body{member->data_offset(), ret.bytes(member->data_offset(), member->size)};
    if (member->header.is_symbol_index()) {
      if (at != REGULAR_MAGIC.size()) return fail(MisplacedSymbolIndex, at);
      ret._symbol_index = body;
    } else if (member->header.is_long_names()) {
      if (ret._long_names) return fail(DuplicateLongNames, at);
      ret._long_names = body;
    } else break;
    c = next;
  }
  ret._first_member = c.position();
  return ret;
}

std::expected<pepp::bts::PackedInputArchive, pepp::bts::ArchiveError>
pepp::bts::PackedInputArchive::open(const std::string &path) {
  auto source = MappedFileSource::open(path);
  if (!source) return fail(CannotOpen, 0);
  return open(std::move(source), path);
}

bits::span<const u8> pepp::bts::PackedInputArchive::bytes(u64 offset, u64 length) const noexcept {
  if (offset > _bytes.size() || length > _bytes.size() - offset) return {};
  return _bytes.subspan(offset, length);
}

std::ranges::subrange<pepp::bts::ArMemberIterator, std::default_sentinel_t>
pepp::bts::PackedInputArchive::members() const {
  ByteCursor cursor(LayoutItem{0, _bytes});
  cursor.offset = _first_member;
  return {ArMemberIterator(*this, cursor), std::default_sentinel};
}

std::ranges::subrange<pepp::bts::ArSymbolIterator, std::default_sentinel_t>
pepp::bts::PackedInputArchive::symbols() const {
  if (!_symbol_index) return {ArSymbolIterator(), std::default_sentinel};
  return {ArSymbolIterator(*this), std::default_sentinel};
}

std::string_view pepp::bts::PackedInputArchive::name(const PackedArMember &member) const {
  return as_string_view(bytes(member.name_offset, member.name_length));
}

bits::span<const u8> pepp::bts::PackedInputArchive::data(const PackedArMember &member) const noexcept {
  return bytes(member.data_offset(), _thin ? 0 : member.size);
}

std::shared_ptr<const pepp::bts::AStorage>
pepp::bts::PackedInputArchive::member_storage(const PackedArMember &member) const {
  return _source->slice(member.data_offset(), _thin ? 0 : member.size);
}

std::expected<std::shared_ptr<const pepp::bts::AElfSource>, pepp::bts::ArchiveError>
pepp::bts::PackedInputArchive::member_source(const PackedArMember &member) const {
  if (_thin && !_path) return fail(ThinMember, member.header_offset);
  else if (_thin) {
    auto source = MappedFileSource::open(thin_member_path(*_path, name(member)));
    if (!source) return fail(CannotOpen, member.header_offset);
    return source;
  }
  return std::make_shared<SliceSource>(_source, member.data_offset(), member.size);
}

std::expected<pepp::bts::PackedArMember, pepp::bts::ArchiveError>
pepp::bts::PackedInputArchive::read_member(ByteCursor &cursor) const {
  const u64 offset = cursor.position();
  auto ret = read_raw(cursor, _thin, _long_names);
  if (!ret) return ret;
  const auto &header = ret->header;
  if (header.is_symbol_index()) return fail(MisplacedSymbolIndex, offset);
  else if (header.is_long_names()) return fail(_long_names ? DuplicateLongNames : MisplacedLongNames, offset);
  else if (!header.date()) return fail(BadDate, offset);
  else if (!header.uid()) return fail(BadUid, offset);
  else if (!header.gid()) return fail(BadGid, offset);
  else if (!header.mode()) return fail(BadMode, offset);
  return ret;
}

std::expected<pepp::bts::AnyInputElf, pepp::bts::ArchiveError>
pepp::bts::open_member_elf(const PackedInputArchive &archive, const PackedArMember &member) {
  const auto source = archive.member_source(member);
  if (!source) return std::unexpected(source.error());
  auto elf = try_open_input_elf(*source);
  if (!elf) return fail(NotElf, member.header_offset);
  return std::move(*elf);
}

std::filesystem::path pepp::bts::thin_member_path(const std::filesystem::path &archive_path,
                                                  std::string_view member_name) {
  // Appending an absolute path replaces the left-hand side.
  return archive_path.parent_path() / member_name;
}
