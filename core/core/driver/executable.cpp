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

#include "core/driver/executable.hpp"
#include <utility>
#include <variant>
#include <fmt/format.h>
#include "core/formats/elf/packed_io.hpp"
#include "core/langs/asmb/debug_line.hpp"
#include "core/link/linker_passes.hpp"
#include "core/link/map_file.hpp"

namespace pepp::tc {

bool ExecutableResult::ok() const { return link && link->ok() && link->output; }

std::shared_ptr<const bts::AElfSource> ExecutableResult::executable() const {
  if (!ok()) return nullptr;
  return std::make_shared<bts::MappedFileSource>(link->output);
}

bts::AnyInputElf ExecutableResult::open() const { return bts::open_input_elf(executable()); }

std::expected<std::vector<std::string>, std::string> ExecutableResult::listing(std::size_t index) {
  auto &assembled = assemblies.at(index);
  if (!assembled.unformatted_listing)
    return std::unexpected("no listing was requested, or assembly failed before codegen");
  if (!ok()) return std::unexpected("the build did not link");
  const auto object = to_input_elf(*assembled.elf());
  auto rows = std::visit([](const auto &file) { return read_listing_rows(*file); }, object);
  if (!rows) return std::unexpected(rows.error());

  // File IDs start at 1, and the sources' objects are the first inputs.
  const bts::ElfFileID file(static_cast<u32>(index + 1));
  const auto absolute = bits::to_underlying(bts::SectionIndices::SHN_ABS);
  for (auto &row : *rows) {
    if (row.section == absolute) continue;
    const auto address = link::section_address(*link, {file, row.section});
    if (!address) return std::unexpected(fmt::format("listing line {} is in a discarded section", row.line));
    row.section = absolute, row.start += *address, row.end += *address;
  }
  const auto &[text, style] = *assembled.unformatted_listing;
  return format_listing(open(), *rows, text, style);
}

ExecutableResult assemble_and_link(ExecutableSpec spec) {
  ExecutableResult ret;
  std::vector<link::LinkInput> inputs;
  for (auto &file : spec.sources) {
    // TODO: remove once ET_REL is the assembler's only output.
    std::visit([](auto &arch) { arch.output_type = bts::ElfFileType::ET_REL; }, file.config);
    auto &assembled = ret.assemblies.emplace_back(assemble(file.config, file.formatting, std::move(file.source)));
    if (auto *object = assembled.elf())
      inputs.push_back({link::LinkInput::Opened{file.name, bts::OpenedInput{to_input_elf(*object)}}});
  }
  if (inputs.size() != spec.sources.size()) return ret;

  for (auto &input : spec.link.inputs) inputs.push_back(std::move(input));
  spec.link.inputs = std::move(inputs);
  ret.link = link::do_link(std::move(spec.link));
  return ret;
}

} // namespace pepp::tc
