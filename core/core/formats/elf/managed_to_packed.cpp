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

#include "core/formats/elf/managed_to_packed.hpp"
#include <algorithm>
#include <functional>
#include <set>
#include <span>
#include <stdexcept>
#include "core/formats/elf/managed_access_strings.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_segment.hpp"

std::vector<pepp::bts::SectionRef>
pepp::bts::garbage_collect_sections(const ManagedElf &elf, const std::function<bool(const ManagedSection &)> &keep) {
  const auto first = ManagedElf::SHN_UNDEF;
  const auto last = elf.last_section();
  std::vector<bool> live(last.value + 1, false);
  // Sections whose info & link fields need to be analyzed for liveness
  std::vector<SectionRef> pending;

  auto mark = [&](SectionRef ref) {
    if (!ref || ref > last || live[ref.value]) return;
    const auto *sec = elf.section(ref);
    // Being referenced cannot resurrect a section that never serializes.
    if (!sec || !sec->is_serialized()) return;
    live[ref.value] = true;
    pending.push_back(ref);
  };

  // Iterate over all possible sections, checking them individually for liveness.
  for (auto it = first; it <= last; ++it)
    if (const auto *sec = elf.section(it); sec && sec->is_serialized() && keep(*sec)) mark(it);

  // Propogate liveness to any sections referenced by a live section's info or link field.
  while (!pending.empty()) {
    const auto *sec = elf.section(pending.back());
    pending.pop_back();
    mark(sec->link);
    if (const auto *target = std::get_if<SectionRef>(&sec->info)) mark(*target);
  }

  std::vector<SectionRef> ret;
  for (auto it = first; it <= last; ++it)
    if (live[it.value]) ret.push_back(it);
  return ret;
}

std::vector<pepp::bts::SectionRef> pepp::bts::garbage_collect_sections(const ManagedElf &elf) {
  return garbage_collect_sections(elf, [](const ManagedSection &) { return true; });
}
