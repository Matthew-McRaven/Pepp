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
#include <limits>
#include <ostream>
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/math/bitmanip/log2.hpp"
#include "core/math/bitmanip/span.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
using namespace pepp::bts;
using enum ArchiveError::Code;

// The size field holds 10 decimal digits.
constexpr u64 max_member_size = 9'999'999'999;
constexpr u8 nul = 0, newline = '\n';

using bits::as_u8_span;
using bits::object_bytes;

std::unexpected<ArchiveError> fail(ArchiveError::Code code, u64 offset) {
  return std::unexpected(ArchiveError{code, offset});
}
} // namespace

u64 pepp::bts::PackedGrowableArchive::SymbolIndex::size() const noexcept {
  const u64 body = sizeof(ub32) * (1 + members.size()) + names.size();
  return bits::align_up(body, 2);
}

u64 pepp::bts::PackedGrowableArchive::first_regular_member_offset() const noexcept {
  u64 ret = PackedInputArchive::REGULAR_MAGIC.size();
  if (!_symbols.members.empty()) ret += ar_header_size + _symbols.size();
  if (_long_names.size() != 0) ret += ar_header_size + _long_names.size();
  return ret;
}

u64 pepp::bts::PackedGrowableArchive::member_size(const Member &member) const noexcept {
  const u64 stored = _thin ? 0 : member.data->size();
  return ar_header_size + bits::align_up(stored, 2);
}

std::expected<u32, pepp::bts::ArchiveError>
pepp::bts::PackedGrowableArchive::add_member(std::string_view name, std::shared_ptr<const AStorage> data,
                                             bits::span<const std::string_view> symbols) {
  // Empty names are invalid. \n is used as a delimiter and can't appear in a name.
  if (name.empty() || name.contains('\n')) {
    u64 end = first_regular_member_offset();
    for (const auto &member : _members) end += member_size(member);
    return fail(name.empty() ? EmptyName : NameHasNewline, end);
  }
  const u32 index = _members.size();
  if (_thin || name.size() >= sizeof(PackedArHeader::ar_name) || name.contains('/')) {
    _members.push_back({"/" + std::to_string(_long_names.entries.size()), std::move(data), {}});
    _long_names.entries.append(as_u8_span(name));
    _long_names.entries.append(as_u8_span("/\n"));
  } else _members.push_back({std::string(name) + "/", std::move(data), {}});
  for (const auto symbol : symbols) {
    _symbols.names.append_cstr(symbol);
    _symbols.members.push_back(index);
  }
  return index;
}

std::expected<std::vector<pepp::bts::LayoutItem>, pepp::bts::ArchiveError>
pepp::bts::PackedGrowableArchive::calculate_layout() {
  // `/` and `//` count their pad bytes in their sizes unlike other members
  const bool has_symbols = !_symbols.members.empty(), has_long_names = _long_names.size() != 0;
  const u64 symbols_at = PackedInputArchive::REGULAR_MAGIC.size();
  const u64 names_at = symbols_at + (has_symbols ? ar_header_size + _symbols.size() : 0);

  // Ensure both special members can fit within the 10-digit size field.
  if (_symbols.size() > max_member_size) return fail(MemberTooLarge, symbols_at);
  else if (_long_names.size() > max_member_size) return fail(MemberTooLarge, names_at);

  // Compute the location in the flattened file for each regular member.
  std::vector<u64> offsets;
  offsets.reserve(_members.size());
  for (u64 at = first_regular_member_offset(); auto &member : _members) {
    if (member.data->size() > max_member_size) return fail(MemberTooLarge, at);
    member.header = PackedArHeader(member.raw_name, member.data->size());
    offsets.push_back(at);
    at += member_size(member);
  }

  std::vector<LayoutItem> ret;
  // Every header starts on an even offset, so an odd end needs a pad byte.
  const auto pad = [&](u64 end, const u8 &byte) {
    if (bits::align_up(end, 2) != end) ret.push_back({end, object_bytes(byte)});
  };
  ret.push_back({0, as_u8_span(_thin ? PackedInputArchive::THIN_MAGIC : PackedInputArchive::REGULAR_MAGIC)});
  if (has_symbols) {
    _symbols.count = static_cast<u32>(_symbols.members.size());
    _symbols.offsets.clear();
    _symbols.offsets.reserve(_symbols.members.size());
    for (const u32 member : _symbols.members) {
      if (offsets[member] > std::numeric_limits<u32>::max()) return fail(NeedsSymbolIndex64, offsets[member]);
      _symbols.offsets.emplace_back(static_cast<u32>(offsets[member]));
    }

    _symbols.header = PackedArHeader("/", _symbols.size(), 0);
    ret.push_back({symbols_at, object_bytes(_symbols.header)});
    const u64 count_at = symbols_at + ar_header_size, offsets_at = count_at + sizeof(ub32);
    ret.push_back({count_at, object_bytes(_symbols.count)});
    const auto bytes = bits::span<const u8>(reinterpret_cast<const u8 *>(_symbols.offsets.data()),
                                            _symbols.offsets.size() * sizeof(ub32));
    ret.push_back({offsets_at, bytes});
    pad(_symbols.names.calculate_layout(ret, offsets_at + bytes.size()), nul);
  }
  if (has_long_names) {
    _long_names.header = PackedArHeader("//", _long_names.size(), std::nullopt);
    ret.push_back({names_at, object_bytes(_long_names.header)});
    pad(_long_names.entries.calculate_layout(ret, names_at + ar_header_size), newline);
  }
  for (size_t it = 0; it < _members.size(); ++it) {
    const auto &member = _members[it];
    ret.push_back({offsets[it], object_bytes(member.header)});
    if (_thin) continue;
    pad(member.data->calculate_layout(ret, offsets[it] + ar_header_size), newline);
  }
  return ret;
}

std::expected<std::vector<u8>, pepp::bts::ArchiveError> pepp::bts::archive_bytes(PackedGrowableArchive &archive) {
  const auto layout = archive.calculate_layout();
  if (!layout) return std::unexpected(layout.error());
  std::vector<u8> ret(size_for_layout(*layout), 0);
  write(ret, *layout);
  return ret;
}

std::expected<void, pepp::bts::ArchiveError> pepp::bts::write_archive(PackedGrowableArchive &archive,
                                                                      std::ostream &out) {
  auto layout = archive.calculate_layout();
  if (!layout) return std::unexpected(layout.error());
  write(out, std::move(*layout));
  return {};
}

std::expected<pepp::bts::PackedInputArchive, pepp::bts::ArchiveError>
pepp::bts::to_input_archive(PackedGrowableArchive &archive) {
  const auto layout = archive.calculate_layout();
  if (!layout) return std::unexpected(layout.error());
  std::vector<char> bytes(size_for_layout(*layout), 0);
  write(std::span<u8>(reinterpret_cast<u8 *>(bytes.data()), bytes.size()), *layout);
  return PackedInputArchive::open(std::make_shared<BufferSource>(std::move(bytes)));
}
