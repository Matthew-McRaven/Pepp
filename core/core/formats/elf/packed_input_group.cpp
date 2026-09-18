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

#include "packed_input_group.hpp"

pepp::bts::AnyElfGroup pepp::bts::to_input_group(AnyInputElf elf) {
  auto visitor = [](auto &file) -> AnyElfGroup {
    using File = typename std::remove_reference_t<decltype(file)>::element_type;
    auto group = std::make_unique<PackedInputElfGroup<File::elf_bits, File::elf_endian>>();
    group->add(std::move(file));
    return group;
  };
  return std::visit(visitor, elf);
}
