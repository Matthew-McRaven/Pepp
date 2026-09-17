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

#include "core/formats/elf/packed_io.hpp"
#include <ostream>

std::vector<u8> pepp::bts::elf_bytes(AnyGrowableElf &elf, const std::vector<SegmentLayoutConstraint> *constraints) {
  auto visitor = [&](auto &file) { return file ? elf_bytes(*file, constraints) : std::vector<u8>{}; };
  return std::visit(visitor, elf);
}

void pepp::bts::write_elf(AnyGrowableElf &elf, std::ostream &out,
                          const std::vector<SegmentLayoutConstraint> *constraints) {
  auto visitor = [&](auto &file) {
    if (file) write_elf(*file, out, constraints);
  };
  std::visit(visitor, elf);
}

pepp::bts::AnyInputElf pepp::bts::to_input_elf(AnyGrowableElf &elf,
                                               const std::vector<SegmentLayoutConstraint> *constraints,
                                               std::optional<std::string> path) {
  auto visitor = [&](auto &file) -> AnyInputElf {
    if (!file) throw std::invalid_argument("to_input_elf: file must be non-null");
    return to_input_elf(*file, constraints, std::move(path));
  };
  return std::visit(visitor, elf);
}
