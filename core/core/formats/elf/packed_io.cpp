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
#include <stdexcept>

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

pepp::bts::AnyInputElf pepp::bts::open_input_elf(std::shared_ptr<const AElfSource> source) {
  using enum ElfIdentifierIndices;
  using enum ElfClass;
  using enum ElfEncoding;
  static constexpr auto ident_size = bits::to_underlying(EI_NIDENT);

  if (!source) throw std::invalid_argument("open_input_elf: source must be non-null");
  const auto ident_slice = source->slice(0, ident_size);
  const auto ident = ident_slice->get(0, ident_slice->size());
  if (ident.size() < ident_size) throw std::runtime_error("not an ELF file");
  const auto at = [&](ElfIdentifierIndices index) { return ident[bits::to_underlying(index)]; };
  if (at(EI_MAG0) != bits::to_underlying(ElfMagic::ELFMAG0) || at(EI_MAG1) != bits::to_underlying(ElfMagic::ELFMAG1) ||
      at(EI_MAG2) != bits::to_underlying(ElfMagic::ELFMAG2) || at(EI_MAG3) != bits::to_underlying(ElfMagic::ELFMAG3))
    throw std::runtime_error("not an ELF file");

  const auto cls = ElfClass(at(EI_CLASS));
  const auto enc = ElfEncoding(at(EI_DATA));
  if (cls == ELFCLASS32 && enc == ELFDATA2LSB) return std::make_unique<PackedInputElfLE32>(source);
  else if (cls == ELFCLASS32 && enc == ELFDATA2MSB) return std::make_unique<PackedInputElfBE32>(source);
  else if (cls == ELFCLASS64 && enc == ELFDATA2LSB) return std::make_unique<PackedInputElfLE64>(source);
  else if (cls == ELFCLASS64 && enc == ELFDATA2MSB) return std::make_unique<PackedInputElfBE64>(source);
  throw std::runtime_error("unsupported ELF class or byte order");
}

pepp::bts::AnyInputElf pepp::bts::open_input_elf(const std::string &path) {
  try {
    return open_input_elf(std::make_shared<MappedFileSource>(MappedFile::open_readonly(path)));
  } catch (const std::runtime_error &e) {
    throw std::runtime_error(path + ": " + e.what());
  }
}
