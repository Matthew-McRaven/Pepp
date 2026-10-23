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
#include <QFile>
#include <algorithm>
#include <catch.hpp>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "core/formats/elf/packed_elf.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
using namespace pepp::bts;
using enum ArchiveError::Code;

// Extract the test data from the Qt resource system.
std::string reference(const QString &name) {
  QFile file(QStringLiteral(":/ar/") + name);
  if (!file.open(QIODevice::ReadOnly)) return {};
  const auto bytes = file.readAll();
  return {bytes.constData(), static_cast<size_t>(bytes.size())};
}

std::expected<PackedArchive, ArchiveError> open_bytes(std::string_view bytes) {
  return PackedArchive::open(std::make_shared<BufferSource>(std::vector<char>(bytes.begin(), bytes.end())));
}

std::string pad(std::string_view text, size_t width) {
  std::string ret(text);
  ret.resize(width, ' ');
  return ret;
}

// A GNU member header with deterministic fields.
std::string header(std::string_view name, std::string_view size, std::string_view mtime = "0",
                   std::string_view uid = "0", std::string_view gid = "0", std::string_view mode = "644",
                   std::string_view fmag = "`\n") {
  return pad(name, 16) + pad(mtime, 12) + pad(uid, 6) + pad(gid, 6) + pad(mode, 8) + pad(size, 10) + std::string(fmag);
}

// Combined header + data, padded to an even length.
std::string member(std::string_view name, std::string_view data) {
  return header(name, std::to_string(data.size())) + std::string(data) + (data.size() % 2 ? "\n" : "");
}

std::string be32(u32 value) {
  const auto bytes = ub32(value).bytes();
  return {bytes.begin(), bytes.end()};
}

std::string arch(std::string_view members) { return std::string(PackedArchive::REGULAR_MAGIC) + std::string(members); }

std::vector<PackedArMember> members_of(const PackedArchive &archive) {
  std::vector<PackedArMember> ret;
  for (const auto &member : archive.members()) ret.push_back(member.value());
  return ret;
}

std::vector<std::string_view> names_of(const PackedArchive &archive, const std::vector<PackedArMember> &members) {
  std::vector<std::string_view> ret;
  for (const auto &member : members) ret.push_back(archive.name(member));
  return ret;
}

// Each symbol with the index of the member whose header it names, or -1 for none.
std::vector<std::pair<std::string_view, i64>> symbols_of(const PackedArchive &archive,
                                                          const std::vector<PackedArMember> &members) {
  std::vector<std::pair<std::string_view, i64>> ret;
  for (const auto &symbol : archive.symbols()) {
    const auto member = std::ranges::find(members, symbol.value().header_offset, &PackedArMember::header_offset);
    ret.emplace_back(symbol.value().name, member == members.end() ? -1 : member - members.begin());
  }
  return ret;
}

// The first error from opening the archive or walking its members and symbols.
std::optional<ArchiveError> first_error(std::string_view bytes) {
  const auto archive = open_bytes(bytes);
  if (!archive) return archive.error();
  for (const auto &member : archive->members())
    if (!member) return member.error();
  for (const auto &symbol : archive->symbols())
    if (!symbol) return symbol.error();
  return std::nullopt;
}

// Member names of data/regular.a and data/thin.a, in file order.
const std::vector<std::string_view> gnu_names{"a.o", "odd.txt", "a_member_with_a_long_name.o"};
// Both definitions of beta are kept. The first is weak in a.o and the second is strong in the long-named member.
const std::vector<std::pair<std::string_view, i64>> gnu_symbols{{"alpha", 0}, {"beta", 0}, {"delta", 2}, {"beta", 2}};
} // namespace

TEST_CASE("PackedArchive on GNU ar inputs", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  const auto bytes = reference("regular.a");
  const auto archive = open_bytes(bytes);
  REQUIRE(archive.has_value());
  CHECK_FALSE(archive->thin());
  const auto members = members_of(*archive);
  CHECK(names_of(*archive, members) == gnu_names);

  SECTION("Deterministic header fields") {
    for (const auto &member : members) {
      CHECK(member.header.date() == 0);
      CHECK(member.header.uid() == 0);
      CHECK(member.header.gid() == 0);
      CHECK(member.header.mode() == 0644);
    }
  }
  SECTION("Member data") {
    for (const auto &member : members) {
      CHECK(bits::as_string_view(archive->data(member)) == bytes.substr(member.data_offset(), member.size));
    }
    CHECK(bits::as_string_view(archive->data(members[1])) == "abc");
    // The pad byte after odd.txt must not shift the next header.
    CHECK(members[2].header_offset == members[1].header_offset + ar_header_size + 4);
  }
  SECTION("Can iterate archive multiple times") {
    const auto offset = &PackedArMember::header_offset;
    CHECK(std::ranges::equal(members_of(*archive), members, {}, offset, offset));
  }
  SECTION("Symbol index") { CHECK(symbols_of(*archive, members) == gnu_symbols); }
  SECTION("Open ELF members") {
    for (const auto index : {0, 2}) {
      const auto &member = members[index];
      const auto elf = open_member_elf(*archive, member);
      REQUIRE(elf.has_value());
      REQUIRE(std::holds_alternative<std::unique_ptr<PackedInputElfLE32>>(*elf));
      const auto &file = *std::get<std::unique_ptr<PackedInputElfLE32>>(*elf);
      CHECK(file.header.e_machine == bits::to_underlying(ElfMachineType::EM_RISCV));
      // Every section lies within the member's bytes.
      for (const auto &shdr : file.section_headers) {
        if (shdr.sh_type == bits::to_underlying(SectionTypes::SHT_NOBITS)) continue;
        CHECK(shdr.sh_offset + shdr.sh_size <= member.size);
      }
    }
    const auto odd = open_member_elf(*archive, members[1]);
    REQUIRE_FALSE(odd.has_value());
    CHECK(odd.error().code == NotElf);
    CHECK(odd.error().offset == members[1].header_offset);
  }
  SECTION("Open archive from file") {
    const std::string path = "packed_archive_regular.a";
    std::ofstream(path, std::ios::binary).write(bytes.data(), bytes.size());
    const auto file = PackedArchive::open(path);
    REQUIRE(file.has_value());
    CHECK(members_of(*file).size() == members.size());
    const auto missing = PackedArchive::open(std::string("packed_archive_missing.a"));
    REQUIRE_FALSE(missing.has_value());
    CHECK(missing.error().code == CannotOpen);
  }
  SECTION("Thin archive") {
    const auto thin = open_bytes(reference("thin.a"));
    REQUIRE(thin.has_value());
    CHECK(thin->thin());
    const auto thin_members = members_of(*thin);
    // GNU puts every thin member name, even a short one, in the // table.
    CHECK(names_of(*thin, thin_members) == gnu_names);
    REQUIRE(thin_members.size() == members.size());
    for (size_t it = 0; it < thin_members.size(); ++it) {
      CHECK(thin->data(thin_members[it]).empty());
      CHECK(thin_members[it].size == members[it].size);
    }
    // Member headers follow each other with no data between them.
    CHECK(thin_members[1].header_offset == thin_members[0].header_offset + ar_header_size);
    // Thin header offsets differ from the regular archive's, but name the same members.
    CHECK(symbols_of(*thin, thin_members) == gnu_symbols);

    const auto elf = open_member_elf(*thin, thin_members[0]);
    REQUIRE_FALSE(elf.has_value());
    CHECK(elf.error().code == ThinMember);
    CHECK(elf.error().offset == thin_members[0].header_offset);
  }
}

TEST_CASE("Unusual, hand-built archives", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  SECTION("Empty") {
    for (const auto magic : {PackedArchive::REGULAR_MAGIC, PackedArchive::THIN_MAGIC}) {
      INFO(magic);
      const auto archive = open_bytes(magic);
      REQUIRE(archive.has_value());
      CHECK(archive->thin() == (magic == PackedArchive::THIN_MAGIC));
      CHECK(archive->members().begin() == std::default_sentinel);
      CHECK(archive->symbols().begin() == std::default_sentinel);
      CHECK_FALSE(archive->symbol_index().has_value());
      CHECK_FALSE(archive->long_names().has_value());
    }
  }
  // Valid one-member archives, matching ar
  SECTION("One member") {
    struct Case {
      std::string name, bytes;
      std::string_view member, data;
    };
    const std::vector<Case> cases{
        {"zero-size member", arch(member("empty.o/", "")), "empty.o", ""},
        {"short name without a slash is space-trimmed", arch(member("foo.o", "xy")), "foo.o", "xy"},
        {"blank uid and gid read as 0", arch(header("foo.o/", "2", "0", "", "") + "xy"), "foo.o", "xy"},
        {"missing final pad byte", arch(header("foo.o/", "3") + "abc"), "foo.o", "abc"},
        {"long name containing a slash", arch(member("//", "dir/a_long_member_name.o/\n") + member("/0", "xy")),
         "dir/a_long_member_name.o", "xy"},
        {"symbol index with no symbols", arch(member("/", be32(0)) + member("foo.o/", "xy")), "foo.o", "xy"},
    };
    for (const auto &c : cases) {
      INFO(c.name);
      const auto archive = open_bytes(c.bytes);
      REQUIRE(archive.has_value());
      const auto members = members_of(*archive);
      REQUIRE(members.size() == 1);
      CHECK(archive->name(members[0]) == c.member);
      CHECK(bits::as_string_view(archive->data(members[0])) == c.data);
      CHECK(members[0].header.uid() == 0);
      CHECK(members[0].header.gid() == 0);
      CHECK(archive->symbols().begin() == std::default_sentinel);
    }
  }
}

// Scenarios from LLVM's test/Object/archive-* malformed-input tests, rebuilt by hand.
TEST_CASE("Reject malformed archives", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  // Offsets of the first member header and its data.
  constexpr u64 first = PackedArchive::REGULAR_MAGIC.size(), first_data = first + ar_header_size;
  const auto foo = member("foo.o/", "xy");
  // A one-symbol index; its 12-byte body puts foo.o at foo_at.
  const std::string sym("sym\0", 4);
  const u64 foo_at = first_data + 12;
  struct Case {
    std::string name, bytes;
    ArchiveError::Code code;
    u64 offset;
  };
  const std::vector<Case> cases{
      {"wrong magic", "!<arhc>\n" + foo, BadMagic, 0},
      {"shorter than the magic", "!<ar", BadMagic, 0},
      {"truncated header", arch(foo.substr(0, 30)), TruncatedHeader, first},
      {"bad terminator", arch(header("foo.o/", "2", "0", "0", "0", "644", "`x") + "xy"), BadFmag, first},
      {"size is not decimal", arch(header("foo.o/", "2a") + "xy"), BadSize, first},
      {"size has a leading space", arch(header("foo.o/", " 2") + "xy"), BadSize, first},
      {"size is empty", arch(header("foo.o/", "") + "xy"), BadSize, first},
      {"size past the end", arch(header("foo.o/", "100") + "xy"), MemberPastEnd, first},
      {"mtime is not decimal", arch(header("foo.o/", "2", "x") + "xy"), BadDate, first},
      {"uid is not decimal", arch(header("foo.o/", "2", "0", "-1") + "xy"), BadUid, first},
      {"gid is not decimal", arch(header("foo.o/", "2", "0", "0", "g") + "xy"), BadGid, first},
      {"mode is not octal", arch(header("foo.o/", "2", "0", "0", "0", "649") + "xy"), BadMode, first},
      {"empty name", arch(member("", "xy")), EmptyName, first},
      {"long name without a // table", arch(member("/0", "xy")), MissingLongNames, first},
      {"long name offset is not decimal", arch(member("//", "a_long_member_name.o/\n") + member("/x", "xy")),
       BadLongNameOffset, first_data + 22},
      {"long name offset past the table", arch(member("//", "a_long_member_name.o/\n") + member("/22", "xy")),
       LongNameOutOfRange, first_data + 22},
      {"long name without /\\n", arch(member("//", "a_long_member_name.o\n") + member("/0", "xy")),
       UnterminatedLongName, first_data + 22},
      {"long name without a newline", arch(member("//", "a_long_member_name.o/") + member("/0", "xy")),
       UnterminatedLongName, first_data + 22},
      {"empty long name", arch(member("//", "/\n") + member("/0", "xy")), EmptyName, first_data + 2},
      {"two // tables", arch(member("//", "x/\n") + member("//", "x/\n")), DuplicateLongNames, first_data + 4},
      {"// after a member", arch(foo + member("//", "x/\n")), MisplacedLongNames, first + foo.size()},
      {"BSD long name", arch(member("#1/8", "foo.o123xy")), UnsupportedBsdName, first},
      {"BSD symbol table", arch(member("__.SYMDEF", "")), UnsupportedBsdSymbolTable, first},
      {"64-bit symbol index", arch(member("/SYM64/", std::string(8, '\0'))), UnsupportedSymbolIndex64, first},
      {"symbol index after a member", arch(foo + member("/", be32(0))), MisplacedSymbolIndex, first + foo.size()},
      {"symbol index after //", arch(member("//", "x/\n") + member("/", be32(0))), MisplacedSymbolIndex,
       first_data + 4},
      {"symbol index without a count", arch(member("/", "ab")), SymbolIndexTruncated, first_data},
      {"symbol index count too large", arch(member("/", be32(3) + be32(foo_at) + sym) + foo), SymbolIndexCountTooLarge,
       first_data},
      {"symbol names truncated", arch(member("/", be32(1) + be32(foo_at) + "sym") + foo), UnterminatedSymbolName,
       first_data + 8},
  };
  for (const auto &c : cases) {
    INFO(c.name);
    const auto error = first_error(c.bytes);
    REQUIRE(error.has_value());
    CHECK(error->code == c.code);
    CHECK(error->offset == c.offset);
  }
}

TEST_CASE("Thin member paths", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  const std::filesystem::path archive_path = std::filesystem::path("lib") / "libfoo.a";
  CHECK(thin_member_path(archive_path, "obj/foo.o") == std::filesystem::path("lib") / "obj/foo.o");
  const auto absolute = std::filesystem::current_path() / "foo.o";
  CHECK(thin_member_path(archive_path, absolute.string()) == absolute);
}
