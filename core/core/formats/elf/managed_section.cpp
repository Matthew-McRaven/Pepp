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

#include "core/formats/elf/managed_section.hpp"

namespace {
using namespace pepp::bts;

struct FileBytes {
  uxword operator()(std::monostate) const { return 0; }
  uxword operator()(const NoBits &) const { return 0; }
  uxword operator()(const RawBytes &raw) const { return raw.bytes.size(); }
  uxword operator()(const ManagedStringTable &table) const { return table.serialized_size(); }
};
} // namespace

pepp::bts::uxword pepp::bts::ManagedSection::file_bytes() const { return std::visit(FileBytes{}, content); }

uxword ManagedSection::memory_bytes() const {
  if (const auto *nobits = std::get_if<NoBits>(&content)) return nobits->size;
  return file_bytes();
}
