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

#include <optional>
#include <utility>
#include "asmb_driver.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb/listing.hpp"
#include "core/langs/asmb/macros.hpp"
#include "core/langs/asmb_riscv/codegen.hpp"
#include "core/langs/asmb_riscv/parser.hpp"
#include "core/langs/asmb_riscv/text_format.hpp"

namespace pepp::tc {

DriverResult assemble_riscv(const RISCVDriverConfig &cfg, const FormattingConfig &fmtcfg, std::string source) {
  DriverResult result;
  auto rv_parser = parser::RISCVParser(support::SeekableData{std::move(source)}, cfg.macros);
  auto sy = rv_parser.symbol_table();
  inject_symdefs(cfg.symdefs, *sy, 4);

  auto program = rv_parser.parse(result.diagnostics);
  if (result.diagnostics.count(Severity::Fatal) > 0) return result;
  if (fmtcfg.source) result.formatted_source = riscv_format_source(program);

  std::optional<parser::MacroComments> comments;
  if (fmtcfg.listing) comments = parser::MacroComments{riscv_format_as_columns, '#'};
  auto flattened = parser::flatten_macros(program, comments);
  auto split = riscv_split_to_sections(result.diagnostics, flattened);
  if (result.diagnostics.count(Severity::Fatal) > 0) return result;

  auto addresses = riscv_assign_addresses(result.diagnostics, split.grouped_ir);
  if (result.diagnostics.count(Severity::Fatal) > 0) return result;
  auto object_code = riscv_to_object_code(result.diagnostics, addresses, split.grouped_ir);
  if (result.diagnostics.count(Severity::Fatal) > 0) return result;
  std::optional<ListingLayout> layout;
  if (fmtcfg.listing) {
    const auto format = [](const LinearIR &line) { return riscv_format_source(&line); };
    layout = layout_listing(flattened, addresses, riscv_listing_bytes_per_line, format);
  }
  // After flattening, so that a definition inside an expansion has its instantiation's line.
  result.set_elf(riscv_to_elf(split.grouped_ir, addresses, object_code, *rv_parser.symbol_table(), cfg.debug_files,
                              parser::defined_macros(program), layout ? &layout->rows : nullptr));
  if (layout)
    result.unformatted_listing = DriverResult::UnformattedListing{std::move(layout->text), riscv_listing_style};
  return result;
}

} // namespace pepp::tc
