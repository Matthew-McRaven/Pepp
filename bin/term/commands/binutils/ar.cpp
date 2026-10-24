/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

#include "ar.hpp"
#include <algorithm>
#include <ctime>
#include <expected>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include "core/formats/ar/extract_symbols.hpp"
#include "core/formats/ar/packed_archive.hpp"
#include "core/formats/elf/packed_access_headers.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
using namespace pepp::bts;
namespace fs = std::filesystem;

using Command = ArTask::Command;
using enum ArTask::Operation;

int fail(std::string_view message) {
  std::cerr << "ar: " << message << "\n";
  return 1;
}

std::string describe(const fs::path &archive, const ArchiveError &error) {
  return fmt::format("{}: {} at offset 0x{:x}", archive.string(), ArchiveError::to_string(error.code), error.offset);
}

// An archive member
struct Entry {
  std::string name;
  std::shared_ptr<const AStorage> data;
  std::optional<AnyInputElf> elf;
};

// Load a named member of a thin archive.
std::expected<Entry, std::string> file_entry(const fs::path &path, std::string name) {
  std::error_code ec;
  if (!fs::is_regular_file(path, ec)) return std::unexpected(path.string() + ": no such file");
  const auto source = std::make_shared<MappedFileSource>(MappedFile::open_readonly(path.string()));
  return Entry{std::move(name), source->slice(0, source->size()), try_open_input_elf(source)};
}

// Extract the filename for a member of a regular or thin archive.
std::string member_name(const fs::path &file, const fs::path &archive, bool thin) {
  if (!thin) return file.filename().string();
  const auto base = archive.parent_path();
  return fs::proximate(file, base.empty() ? fs::path(".") : base).generic_string();
}

// For each element of requested, see if one of the archive members, named by name_of, matches it.
// If so, return the index of the member, else nullopt.
template <typename Member, typename NameOf>
std::vector<std::optional<size_t>> select_members(const std::vector<Member> &members, NameOf name_of,
                                                  const std::vector<std::string> &requested, const fs::path &archive,
                                                  bool thin) {
  std::vector<std::optional<size_t>> ret;
  std::vector<bool> taken(members.size());
  for (const auto &file : requested) {
    const auto name = member_name(file, archive, thin);
    std::optional<size_t> hit;
    for (size_t it = 0; it < members.size() && !hit; ++it)
      if (!taken[it] && std::invoke(name_of, members[it]) == name) hit = it;
    if (hit) taken[*hit] = true;
    ret.push_back(hit);
  }
  return ret;
}

void warn_missing(const std::vector<std::string> &requested, const std::vector<std::optional<size_t>> &selected) {
  for (size_t it = 0; it < requested.size(); ++it)
    if (!selected[it]) fail(fmt::format("no entry {} in archive", requested[it]));
}

std::string permissions(u32 mode) {
  std::string ret;
  for (int shift = 6; shift >= 0; shift -= 3) {
    ret += mode >> shift & 4 ? 'r' : '-';
    ret += mode >> shift & 2 ? 'w' : '-';
    ret += mode >> shift & 1 ? 'x' : '-';
  }
  return ret;
}

std::string date(u64 seconds) {
  const auto time = static_cast<std::time_t>(seconds);
  char text[32];
  std::strftime(text, sizeof(text), "%b %e %H:%M %Y", std::localtime(&time));
  return text;
}

// Handles t, p, and x that read the archive without changing it.
int read_members(const Command &command, const fs::path &path, const std::vector<std::string> &names) {
  const auto archive = PackedInputArchive::open(path.string());
  if (!archive) return fail(describe(path, archive.error()));
  else if (command.operation != List && archive->thin())
    return fail(path.string() + ": a thin archive stores no members");
  std::vector<PackedArMember> members;
  for (const auto &member : archive->members()) {
    if (!member) return fail(describe(path, member.error()));
    members.push_back(*member);
  }
  const auto name_of = [&](const PackedArMember &member) { return archive->name(member); };
  const auto selected = select_members(members, name_of, names, path, archive->thin());
  warn_missing(names, selected);
  // No names select every member.
  std::vector<bool> chosen(members.size(), names.empty());
  for (const auto &index : selected)
    if (index) chosen[*index] = true;

  for (size_t index = 0; index < members.size(); ++index) {
    if (!chosen[index]) continue;
    const auto &member = members[index];
    const auto name = name_of(member);
    const auto &header = member.header;
    const auto data = bits::as_string_view(archive->data(member));
    if (command.operation == List) {
      if (command.verbose) {
        const auto mode = permissions(header.mode().value_or(0)), when = date(header.date().value_or(0));
        std::cout << fmt::format("{} {}/{} {:>6} {} {}\n", mode, header.uid().value_or(0), header.gid().value_or(0),
                                 member.size, when, name);
      } else std::cout << name << "\n";
    } else if (command.operation == Print) {
      if (command.verbose) std::cout << "\n<" << name << ">\n\n";
      std::cout.write(data.data(), static_cast<std::streamsize>(data.size()));
    } else {
      if (command.verbose) std::cout << "x - " << name << "\n";
      const auto out = fs::path(name).filename();
      std::ofstream file(out, std::ios::binary);
      file.write(data.data(), static_cast<std::streamsize>(data.size()));
      file.close();
      if (!file) return fail(out.string() + ": write failed");
      std::error_code ec;
      fs::permissions(out, static_cast<fs::perms>(header.mode().value_or(0644) & 0777), ec);
    }
  }
  return 0;
}

// The members of an existing archive mapped into memory. Converted to ELF files where possible.
std::expected<std::vector<Entry>, std::string> archive_entries(const PackedInputArchive &archive,
                                                               const fs::path &path) {
  std::vector<Entry> ret;
  for (const auto &member : archive.members()) {
    if (!member) return std::unexpected(describe(path, member.error()));
    std::string name(archive.name(*member));
    if (archive.thin()) {
      auto entry = file_entry(thin_member_path(path, name), name);
      if (!entry) return std::unexpected(entry.error());
      ret.push_back(std::move(*entry));
      continue;
    }
    auto elf = open_member_elf(archive, *member);
    ret.push_back({std::move(name), archive.member_storage(*member),
                   elf ? std::optional(std::move(*elf)) : std::nullopt});
  }
  return ret;
}

// Write out the given entries to a temporary file, returning the path of that temporary on success.
std::expected<fs::path, std::string> write_entries(const fs::path &path, const std::vector<Entry> &entries, bool thin,
                                                   bool index) {
  PackedGrowableArchive out(thin);
  for (const auto &entry : entries) {
    std::vector<std::string_view> symbols;
    if (index && entry.elf) symbols = defined_globals(*entry.elf);
    if (const auto added = out.add_member(entry.name, entry.data, symbols); !added)
      return std::unexpected(describe(path, added.error()));
  }
  auto tmp = path;
  tmp += ".tmp";
  std::ofstream file(tmp, std::ios::binary);
  if (!file) return std::unexpected(tmp.string() + ": cannot open for writing");
  const auto written = write_archive(out, file);
  file.close();
  if (!written || !file) {
    std::error_code ec;
    fs::remove(tmp, ec);
    return std::unexpected(written ? tmp.string() + ": write failed" : describe(path, written.error()));
  }
  return tmp;
}

// Handle d, q, r, and s. All modify the archive in-place. Returns the path of a temporary file containing the rebuilt
// archive. Callers should replace the input file with that temporary by renaming it.
std::expected<fs::path, std::string> modify(const Command &command, const fs::path &path,
                                            const std::vector<std::string> &files) {
  std::error_code ec;
  std::optional<PackedInputArchive> archive;
  if (fs::exists(path, ec)) {
    auto opened = PackedInputArchive::open(path.string());
    if (!opened) return std::unexpected(describe(path, opened.error()));
    archive = std::move(*opened);
  } else if (command.operation == Delete || command.operation == Index)
    return std::unexpected(path.string() + ": no such file");
  else if (!command.quiet_create) std::cerr << "ar: creating " << path.string() << "\n";
  // "thinness" must match that of the existing archive.
  if (archive && command.thin != archive->thin())
    return std::unexpected(path.string() + (command.thin ? ": cannot convert a regular archive to a thin one"
                                                      : ": cannot convert a thin archive to a regular one"));
  const bool thin = command.thin;
  // Always update the symbol index unless explicitly disabled by S.
  const bool index = !command.no_index;

  std::vector<Entry> entries;
  if (archive) {
    auto existing = archive_entries(*archive, path);
    if (!existing) return std::unexpected(existing.error());
    entries = std::move(*existing);
  }
  // Computed before any entries are modified.
  auto selected = select_members(entries, &Entry::name, files, path, thin);
  if (command.operation == Delete) {
    warn_missing(files, selected);
    // Erasing from the back keeps the remaining indices valid; nullopt sorts last.
    std::ranges::sort(selected, std::greater{});
    for (const auto &index : selected) {
      if (!index) continue;
      if (command.verbose) std::cout << "d - " << entries[*index].name << "\n";
      entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(*index));
    }
  } else if (command.operation == QuickAppend || command.operation == Replace) {
    for (size_t it = 0; it < files.size(); ++it) {
      const auto name = member_name(files[it], path, thin);
      auto entry = file_entry(files[it], name);
      if (!entry) return std::unexpected(entry.error());
      // q always appends; r replaces the member the file selects, or appends the file if it selects none.
      if (command.operation == Replace && selected[it]) {
        if (command.verbose) std::cout << "r - " << name << "\n";
        entries[*selected[it]] = std::move(*entry);
      } else {
        if (command.verbose) std::cout << "a - " << name << "\n";
        entries.push_back(std::move(*entry));
      }
    }
  }
  return write_entries(path, entries, thin, index);
}

int execute(const Command &command) {
  const fs::path path = command.archive;
  if (command.operation == List || command.operation == Print || command.operation == Extract) {
    std::error_code ec;
    if (!fs::exists(path, ec)) return fail(path.string() + ": no such file");
    return read_members(command, path, command.members);
  }
  // Every memory-mapping over input files are released when modify returns. We can rename over the original archive.
  const auto rebuilt = modify(command, path, command.members);
  if (!rebuilt) return fail(rebuilt.error());
  std::error_code ec;
  fs::rename(*rebuilt, path, ec);
  if (!ec) return 0;
  const auto message = path.string() + ": " + ec.message();
  fs::remove(*rebuilt, ec);
  return fail(message);
}
} // namespace

ArTask::ArTask(Command &command, QObject *parent) : Task(parent), _command(command) {}

void ArTask::run() {
  if (!_command.operation) return emit finished(fail("no operation specified; see ar --help"));
  return emit finished(execute(_command));
}
